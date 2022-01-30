#pragma once

#include <vector>

#include "absl/strings/str_format.h"
#include "color_guess.h"
#include "state.h"

namespace wordle {

template <typename MaskType>
struct Branch {
  Branch() = default;
  Branch(const Branch&) = delete;
  Branch(Branch&&) = default;
  Branch& operator=(const Branch&) = delete;
  Branch& operator=(Branch&&) = default;

  Colors colors;
  MaskType mask;
};

template <typename MaskType>
struct Partition {
  Partition() = default;
  Partition(const Partition&) = delete;
  Partition(Partition&&) = default;
  Partition& operator=(const Partition&) = delete;
  Partition& operator=(Partition&&) = default;

  using BranchType = Branch<MaskType>;
  Word word;
  std::vector<Branch<MaskType>> branches;
};

using FullPartition = Partition<State>;
using FullBranch = Branch<State>;

std::vector<FullPartition> SubPartitions(const State& input);

template <int N>
class ThinPartitionMap {
 public:
  ThinPartitionMap(const State& mask);

 private:
  struct ShiftMask {
    uint64_t select_mask;
    int source_word;
    int target_word;
    int shift;
  };

  std::vector<ShiftMask> reduce_steps_;
  std::vector<Word> words_;
  std::vector<Partition<ThinState<N>>> all_partitions_;
};

template <int N>
ThinPartitionMap<N>::ThinPartitionMap(const State& mask) {
  if (mask.count() > 64 * N) {
    __builtin_trap();
  }
  int cur_target = 0;
  int cur_shift = 0;
  for (int word_i = 0; word_i < State::kNumWords; ++word_i) {
    uint64_t this_word = mask.array()[word_i];
  retry:
    if (!this_word) continue;
    ShiftMask sm;
    sm.select_mask = 0;
    sm.source_word = word_i;
    sm.target_word = cur_target;
    sm.shift = cur_shift;
    while (this_word && cur_shift < 64) {
      int bit_index = absl::countr_zero(this_word);
      uint64_t bit = uint64_t{1} << bit_index;
      words_.push_back(Word(64 * word_i + bit_index));
      this_word ^= bit;
      sm.select_mask |= bit;
      cur_shift += 1;
    }
    reduce_steps_.push_back(sm);
    if (cur_shift == 64) {
      cur_target += 1;
      cur_shift = 0;
      goto retry;
    }
  }
}

}  // namespace wordle