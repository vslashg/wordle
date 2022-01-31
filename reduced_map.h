#pragma once

#include <immintrin.h>

#include <array>
#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

#include "raw_data.h"
#include "state.h"

namespace wordle {

class BitReducer {
 public:
  BitReducer(const std::array<uint64_t, 37>& mask);
  BitReducer(const State& state) : BitReducer(state.array()) {}

  template <int N>
  std::array<uint64_t, N> Reduce(const std::array<uint64_t, 37>& mask) const;
  template <int N>
  std::array<uint64_t, N> Reduce(const State& state) const {
    return Reduce<N>(state.array());
  }

 private:
  struct ReduceStep {
    uint64_t select_mask;
    int source_word;
    int target_word;
    int shift;
  };
  std::vector<ReduceStep> reduce_steps_;
  std::vector<Word> words_;
};

struct ReducedBranch {
  Colors colors;
  uint16_t vowel_index;
  uint16_t consonant_index;
  uint16_t num_bits;

  struct LongFirst {
    bool operator()(const ReducedBranch& lhs, const ReducedBranch& rhs) const {
      return std::make_tuple(-int(lhs.num_bits), lhs.vowel_index,
                             lhs.consonant_index) <
             std::make_tuple(-int(rhs.num_bits), rhs.vowel_index,
                             rhs.consonant_index);
    }
  };
  struct ShortFirst {
    bool operator()(const ReducedBranch& lhs, const ReducedBranch& rhs) const {
      return std::make_tuple(int(lhs.num_bits), lhs.vowel_index,
                             lhs.consonant_index) <
             std::make_tuple(int(rhs.num_bits), rhs.vowel_index,
                             rhs.consonant_index);
    }
  };
  struct MaskEq {
    bool operator()(const ReducedBranch& lhs, const ReducedBranch& rhs) const {
      return lhs.vowel_index == rhs.vowel_index &&
             lhs.consonant_index == rhs.consonant_index;
    }
  };
};

template <int num_words>
class ReducedMaskTable {
 public:
  template <typename Source>
  ReducedMaskTable(const Source& s, const BitReducer& reducer) {
    std::deque<std::array<uint64_t, num_words>> reduced_masks_deque;
    int count = std::end(s) - std::begin(s);
    std::map<std::array<uint64_t, num_words>, int> lookup;
    for (int idx = 0; idx < count; ++idx) {
      std::array<uint64_t, num_words> reduced =
          reducer.Reduce<num_words>(s[idx]);
      auto res = lookup.try_emplace(reduced, lookup.size());
      if (res.second) {  // insertion successful
        reduced_masks_deque.push_back(reduced);
      }
      full_to_reduced_index_map_.push_back(res.first->second);
    }
    reduced_masks_ = std::vector<std::array<uint64_t, num_words>>(
        reduced_masks_deque.begin(), reduced_masks_deque.end());
  }

  int size() const { return reduced_masks_.size(); }

  int ReduceFullIndex(int full_index) const {
    return full_to_reduced_index_map_[full_index];
  }

  const std::array<uint64_t, num_words>& LookupByReducedIndex(
      int reduced_index) const {
    return reduced_masks_[reduced_index];
  }

 private:
  std::vector<std::array<uint64_t, num_words>> reduced_masks_;
  std::vector<int> full_to_reduced_index_map_; 
};

template <int num_words>
class ReducedPartitions {
 public:
  struct Guess {
    Word word;
    std::vector<ReducedBranch> branches;
  };

  ReducedPartitions(const State& mask)
      : reducer_(mask),
        vowel_masks_(raw::vowel_masks, reducer_),
        consonant_masks_(raw::consonant_masks, reducer_) {
    std::cout << "Vowel masks reduced from " << 13912 << " to "
              << vowel_masks_.size() << "\n";
    std::cout << "Consonant masks reduced from " << 36237 << " to "
              << consonant_masks_.size() << "\n";
    
    int total_branch_count_debug = 0;
    int reduced_branch_count_debug = 0;
    for (const raw::Guess& raw_guess : raw::guesses) {
      Guess reduced_guess;
      reduced_guess.word = raw_guess.word;
      for (const raw::Indices& branch : raw_guess.branches) {
        ReducedBranch br = Reduce(branch);
        ++total_branch_count_debug;
        if (br.num_bits != 0 && br.num_bits < mask.count()) {
          reduced_guess.branches.push_back(br);
          ++reduced_branch_count_debug;
        }
      }
      if (!reduced_guess.branches.empty()) {
        std::sort(reduced_guess.branches.begin(), reduced_guess.branches.end(),
                  ReducedBranch::ShortFirst{});
        guesses_.push_back(reduced_guess);
      }
    }
    auto guess_lt = [](const Guess& lhs, const Guess& rhs) {
      return std::lexicographical_compare(
          lhs.branches.begin(), lhs.branches.end(), rhs.branches.begin(),
          rhs.branches.end(), ReducedBranch::LongFirst{});
    };
    std::sort(guesses_.begin(), guesses_.end(), guess_lt);

    auto guess_eq = [](const Guess& lhs, const Guess& rhs) {
      return lhs.branches.size() == rhs.branches.size() &&
             std::equal(lhs.branches.begin(), lhs.branches.end(),
                        rhs.branches.begin(), ReducedBranch::MaskEq{});
    };
    guesses_.erase(std::unique(guesses_.begin(), guesses_.end(), guess_eq),
                   guesses_.end());
    std::cout << "Guesses reduced from " << kNumTargets + kNumNonTargets
              << " to " << guesses_.size() << "\n";
    std::cout << "Total branches reduced from " << total_branch_count_debug
              << " to " << reduced_branch_count_debug << "\n";
  }

 private:
  ReducedBranch Reduce(const raw::Indices& ri) const {
    ReducedBranch reduced;
    reduced.colors = ri.colors;
    reduced.vowel_index = vowel_masks_.ReduceFullIndex(ri.vowel_index);
    reduced.consonant_index =
        consonant_masks_.ReduceFullIndex(ri.consonant_index);
    reduced.num_bits = 0;

    const std::array<uint64_t, num_words>& vowel_mask =
        vowel_masks_.LookupByReducedIndex(reduced.vowel_index);
    const std::array<uint64_t, num_words>& consonant_mask =
        consonant_masks_.LookupByReducedIndex(reduced.consonant_index);
    for (int i = 0; i < num_words; ++i) {
      reduced.num_bits += absl::popcount(vowel_mask[i] & consonant_mask[i]);
    }
    return reduced;
  }

  BitReducer reducer_;
  ReducedMaskTable<num_words> vowel_masks_;
  ReducedMaskTable<num_words> consonant_masks_;
  std::vector<Guess> guesses_;
};

////////

BitReducer::BitReducer(const std::array<uint64_t, 37>& mask) {
  int cur_target = 0;
  int cur_shift = 0;
  for (int word_i = 0; word_i < State::kNumWords; ++word_i) {
    uint64_t this_word = mask[word_i];
  retry:
    if (!this_word) continue;
    ReduceStep step;
    step.select_mask = 0;
    step.source_word = word_i;
    step.target_word = cur_target;
    step.shift = cur_shift;
    while (this_word && cur_shift < 64) {
      int bit_index = absl::countr_zero(this_word);
      uint64_t bit = uint64_t{1} << bit_index;
      words_.push_back(Word(64 * word_i + bit_index));
      this_word ^= bit;
      step.select_mask |= bit;
      cur_shift += 1;
    }
    reduce_steps_.push_back(step);
    if (cur_shift == 64) {
      cur_target += 1;
      cur_shift = 0;
      goto retry;
    }
  }
}

template <int N>
std::array<uint64_t, N> BitReducer::Reduce(
    const std::array<uint64_t, 37>& mask) const {
  if (N * 64 < words_.size()) {
    __builtin_trap();
  }
  std::array<uint64_t, N> out = {0};
  for (const ReduceStep& step : reduce_steps_) {
    uint64_t bits = _pext_u64(mask[step.source_word], step.select_mask);
    bits <<= step.shift;
    out[step.target_word] |= bits;
  }
  return out;
}

}  // namespace wordle