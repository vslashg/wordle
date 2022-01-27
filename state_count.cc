#include <string_view>

#include "partition_map.h"
#include "state.h"

using namespace wordle;

const PartitionMap& pm = PartitionMap::Singleton();

void concoct(int cap) {
  std::set<wordle::State> all_masks;
  int full_count = 0;
  for (Partition& p : pm.SubPartitions(wordle::State::AllBits())) {
    full_count += p.branches.size();
    for (Branch& b : p.branches) {
      all_masks.emplace(std::move(b.mask));
    }
  }
  std::cout << full_count << " branches, " << all_masks.size() << " unique\n";
  while ((*all_masks.begin()).count() < cap) {
    all_masks.erase(all_masks.begin());
  }
  std::cout << all_masks.size() << " of those have " << cap
            << " bits or more set\n";
  std::set<wordle::State> sm;
  sm.emplace(State::MakeAllBits());
  int counted = 0;
  while (!sm.empty()) {
    int cur_size = sm.rbegin()->count();
    for (const State& mask : all_masks) {
      State combined = mask & *sm.rbegin();
      if (combined.count() >= cap && combined.count() < cur_size) {
        sm.emplace(std::move(combined));
      }
    }
    sm.erase(std::prev(sm.end()));
    ++counted;
    if (sm.empty() || (counted % 100) == 0) {
      printf("% 8d counted, % 8d in state machine, size %4d\r", counted,
             int(sm.size()), cur_size);
      fflush(stdout);    
    }
  }
  printf("\n\n%d\n", counted);
}

int main() {
  concoct(257);
  concoct(238);
}