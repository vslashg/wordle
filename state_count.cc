#include <optional>
#include <string_view>
#include <thread>

#include "partition_map.h"
#include "state.h"
#include "score.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"

using namespace wordle;

// low_len and high_len are inclusive
void concoct(int num_threads, int low_len, int high_len, unsigned bin_begin,
             unsigned bin_end, unsigned num_bins) {
  std::set<wordle::State> all_masks;
  int full_count = 0;
  for (FullPartition& p : SubPartitions(wordle::State::AllBits())) {
    full_count += p.branches.size();
    for (FullBranch& b : p.branches) {
      all_masks.emplace(std::move(b.mask));
    }
  }
  while ((*all_masks.begin()).count() < low_len) {
    all_masks.erase(all_masks.begin());
  }

  struct LockedStates {
    absl::Mutex mu;
    absl::flat_hash_set<wordle::State> s;
  };
  std::deque<LockedStates> sm;
  absl::flat_hash_set<uint64_t> rapidash;
  sm.resize(kNumTargets + 1);

  absl::Mutex every_state_mu;
  std::deque<wordle::State> every_state;

  sm[kNumTargets].s.insert(State::MakeAllBits());
  int counted = 0;

  while (!sm.empty()) {
    if (sm.back().s.empty()) {
      sm.pop_back();
      continue;
    }
    absl::Mutex current_items_mu;
    absl::flat_hash_set<wordle::State> current_items = std::move(sm.back().s);
    sm.pop_back();
    int current_size = sm.size();
    std::vector<std::thread> threads;
    int work_units = 0;
    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&]{
        while (true) {
          std::optional<wordle::State> s;
          {
            absl::MutexLock lock(&current_items_mu);
            if (current_items.empty()) {
              return;
            }
            s = std::move(current_items.extract(current_items.begin()).value());
            auto ins = rapidash.insert(s->Rapidash());
            if (!ins.second) {
              printf("\n\nCollision at %lx\n\n", *ins.first);
              exit(1);
            }
            ++counted;
          }
          for (const State& mask : all_masks) {
            State combined = mask & *s;
            if (combined.count() >= low_len &&
                combined.count() < current_size) {
              absl::MutexLock lock(&sm[combined.count()].mu);
              sm[combined.count()].s.insert(std::move(combined));
            }
          }
          unsigned this_bin = s->Rapidash() % num_bins;
          if (s->count() >= low_len && s->count() <= high_len &&
              this_bin >= bin_begin && this_bin < bin_end && !IsCached(*s)) {
            absl::MutexLock lock(&every_state_mu);
            every_state.push_back(*std::move(s));
            ++work_units;
          }
        }
      });
    }
    for (int i = 0; i < num_threads; ++i) {
      threads[i].join();
    }
    fprintf(
        stderr,
        "% 8d counted, % 2d duplicates, size %4d\n",
        counted, int(counted - rapidash.size()), current_size);
    if (work_units > 0) {
      fprintf(stderr, "  % 8d work units enqueued\n", work_units);
    }
    fflush(stderr);
  }
  fprintf(stderr, "\n\nTotal states seen = %d\n", counted);
  int number_in_work = every_state.size();
  int number_complete = 0;
  fprintf(stderr, "Work enqueued = %d\n", number_in_work);
  int best_wu;
  if (number_in_work) {
    best_wu = every_state.back().count();
    fprintf(stderr, "Bottom end = %d\n", best_wu);
  } else {
    return;
  }

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&]{
      while (true) {
        std::optional<wordle::State> s;
        {
          absl::MutexLock lock(&every_state_mu);
          if (every_state.empty()) {
            return;
          }
          s = std::move(every_state.back());
          every_state.pop_back();
        }
        ScoreResult sr = ScoreState(*s);
        AddHash(s->Rapidash(), sr);
        {
          absl::MutexLock lock(&every_state_mu);
          printf(":: %016lx %d %s\n", s->Rapidash(), sr.first,
                sr.second.ToString());
          fflush(stdout);
          ++number_complete;
          double percent = 100.0 * number_complete / number_in_work;
          fprintf(stderr, "%7d/%7d %02.3f%%\r", number_complete, number_in_work,
                  percent);
          if (s->count() > best_wu) {
            best_wu = s->count();
            fprintf(stderr, "\nStarting on size %d\n", best_wu);
          }
          fflush(stderr);
        }
      }
    });
  }
  for (int i = 0; i < num_threads; ++i) {
    threads[i].join();
  }
}

int main(int argc, char** argv) {
  int threads, low_len, high_len;
  unsigned bin_begin, bin_end, num_bins;
  if (argc != 7 || !absl::SimpleAtoi(argv[1], &threads) ||
      !absl::SimpleAtoi(argv[2], &low_len) ||
      !absl::SimpleAtoi(argv[3], &high_len) ||
      !absl::SimpleAtoi(argv[4], &bin_begin) ||
      !absl::SimpleAtoi(argv[5], &bin_end) ||
      !absl::SimpleAtoi(argv[6], &num_bins) || bin_begin >= bin_end ||
      bin_end > num_bins) {
    std::cerr
        << "Usage: " << argv[0]
        << " <threads> <low_len> <high_len> <bin_begin> <bin_end> <num_bins>\n";
    return 1;
  }
  std::cerr << "Reading seed data\n";
  int count = 0;
  while (std::cin) {
    std::string line;
    std::getline(std::cin, line);
    if (line.empty() || line[0] != ':') {
      break;
    }
    uint64_t hash;
    int score;
    char word[6] = ".....";
    if (sscanf(line.c_str(), ":: %lx %d %5c", &hash, &score, word) != 3) {
      std::cerr << "failed on " << line << "\n";
      break;
    }
    Word w(word);
    if (!w.IsValid()) {
      std::cerr << "failed on " << line << "\n";
      break;
    }
    if (!AddHash(hash, ScoreResult{score, word})) {
      std::cerr << "failed to insert " << line << "\n";
      break;
    }
    ++count;
  }
  std::cerr << count << " seeds read\n";
  concoct(threads, low_len, high_len, bin_begin, bin_end, num_bins);
}