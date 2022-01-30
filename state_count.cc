#include <optional>
#include <string_view>
#include <thread>

#include "partition_map.h"
#include "state.h"
#include "absl/synchronization/mutex.h"

using namespace wordle;

void concoct(int cap) {
  std::set<wordle::State> all_masks;
  int full_count = 0;
  for (FullPartition& p : SubPartitions(wordle::State::AllBits())) {
    full_count += p.branches.size();
    for (FullBranch& b : p.branches) {
      all_masks.emplace(std::move(b.mask));
    }
  }
  std::cout << full_count << " branches, " << all_masks.size() << " unique\n";
  while ((*all_masks.begin()).count() < cap) {
    all_masks.erase(all_masks.begin());
  }
  std::cout << all_masks.size() << " of those have " << cap
            << " bits or more set\n";

  struct LockedStates {
    absl::Mutex mu;
    std::set<wordle::State> s;
  };
  std::deque<LockedStates> sm;
  struct Rapidash {
    int count;
//    State::Array ary;
  };
  std::map<uint64_t, Rapidash> rapidash;
  sm.resize(kNumTargets + 1);

  sm[kNumTargets].s.insert(State::MakeAllBits());
  int counted = 0;

  while (!sm.empty()) {
    if (sm.back().s.empty()) {
      sm.pop_back();
      continue;
    }
    absl::Mutex current_items_mu;
    std::set<wordle::State> current_items = std::move(sm.back().s);
    sm.pop_back();
    int current_size = sm.size();
    std::vector<std::thread> threads;
    for (int i = 0; i < 32; ++i) {
      threads.emplace_back([&]{
        while (true) {
          std::optional<wordle::State> s;
          {
            absl::MutexLock lock(&current_items_mu);
            if (current_items.empty()) return;
            s = std::move(current_items.extract(current_items.begin()).value());
            auto ins = rapidash.try_emplace(s->Rapidash());
            if (!ins.second) {
              printf("\nCollision at %lx, was %d, now %d\n", ins.first->first,
                     ins.first->second.count, s->count());
              /*
              for (int i = 0; i < State::kNumWords; ++i) {
                uint64_t lhs = ins.first->second.ary[i];
                uint64_t rhs = s->array()[i];
                printf("% 3i %lx %lx %lx\n", i, lhs, rhs, lhs ^ rhs);
              }
              */
            } else {
              // ins.first->second.ary = s->array();
            }
            ++counted;
          }
          for (const State& mask : all_masks) {
            State combined = mask & *s;
            if (combined.count() >= cap && combined.count() < current_size) {
              absl::MutexLock lock(&sm[combined.count()].mu);
              sm[combined.count()].s.insert(std::move(combined));
            }
          }
        }
      });
    }
    for (int i = 0; i < 32; ++i) {
      threads[i].join();
    }
    printf("% 8d counted, % 2d duplicates, % 8d in state machine, size %4d\r",
           counted, int(counted - rapidash.size()), int(sm.size()),
           current_size);
    fflush(stdout);
  }
  printf("\n\n%d\n", counted);
}

int main() {
  concoct(257);
}