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
void concoct(int num_threads, int low_len) {
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
    fflush(stderr);
  }
  fprintf(stderr, "\n\nTotal states seen = %d\n", counted);
}

int main(int argc, char** argv) {
  int threads, low_len;
  if (argc != 3 || !absl::SimpleAtoi(argv[1], &threads) ||
      !absl::SimpleAtoi(argv[2], &low_len)) {
    std::cerr << "Usage: " << argv[0] << " <threads> <low_len>\n";
    return 1;
  }
  concoct(threads, low_len);
}