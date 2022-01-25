#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>

#include "absl/numeric/bits.h"
#include "dictionary.h"

namespace wordle {

class State {
 public:
  State() : words_(std::make_unique<Array>()), num_bits_(0) {
    std::fill(words_->begin(), words_->end(), uint64_t{0});
  }
  State(const State&) = delete;
  State(State&&) = default;
  State& operator=(const State&) = delete;
  State& operator=(State&&) = default;

  static const State& AllBits() {
    static State ab = [] {
      State t;
      for (int i = 0; i < kNumTargets; ++i) {
        t.SetBit(i);
      }
      return t;
    }();
    return ab;
  }

  friend State operator&(const State& lhs, const State& rhs) {
    State ans;
    ans.num_bits_ = 0;
    ans.exemplar_ = -1;
    for (int i = 0; i < kNumWords; ++i) {
      uint64_t word = (*lhs.words_)[i] & (*rhs.words_)[i];
      (*ans.words_)[i] = word;
      ans.num_bits_ += __builtin_popcountll(word);
      if (word != 0 && ans.exemplar_ == -1) {
        ans.exemplar_ = (64 * i) + absl::countr_zero(word);
      }
    }
    return ans;
  }

  void SetBit(int idx) {
    const int word_idx = idx / 64;
    const int pos_idx = idx % 64;
    uint64_t& word = (*words_)[word_idx];
    const uint64_t mask = uint64_t{1} << pos_idx;
    if (!(word & mask)) {
      word |= mask;
      ++num_bits_;
    }
    exemplar_ = std::min(exemplar_, idx);
  }

  const char* Exemplar() const {
    return (exemplar_ == kNumTargets) ? "NONE" : targets[exemplar_];
  }

  int count() const { return num_bits_; }
  bool empty() const { return num_bits_ == 0; }
  int exemplar_index() const { return exemplar_; }

  bool operator==(const State& r) const {
    return num_bits_ == r.num_bits_ && exemplar_ == r.exemplar_ &&
           *words_ == *r.words_;
  }
  bool operator!=(const State& r) const { return !(*this == r); }
  template <typename H>
  friend H AbslHashValue(H h, const State& s) {
    return H::combine(std::move(h), *s.words_);
  }

 private:
  static constexpr int kNumWords = (kNumTargets + 63) / 64;
  using Array = std::array<uint64_t, kNumWords>;
  std::unique_ptr<Array> words_;
  int num_bits_;
  int exemplar_ = kNumTargets;
};

}  // namespace wordle