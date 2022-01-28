#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <memory>

#include "absl/hash/hash.h"
#include "absl/numeric/bits.h"
#include "dictionary.h"

namespace wordle {

using StateId = unsigned __int128;

class State {
 public:
  State(const std::bitset<kNumTargets>& b) : State() {
    for (int i = 0; i < kNumTargets; ++i) {
      if (b[i]) {
        SetBit(i);
      }
    }
    bits_hash_ = absl::HashOf(*words_);
  }
  State(const State&) = delete;
  State(State&&) = default;
  State& operator=(const State&) = delete;
  State& operator=(State&&) = default;

  // Return an integer ID uniquely representing this state.  For two states
  // with different count()s, the type with the higher count() has the higher
  // StateId.  (This is intended to allow for useful sort ordering.)
  StateId ToStateId() const {
    return (StateId(num_bits_) << 96) | (StateId(min_bit_index_) << 80) |
           (StateId(max_bit_index_) << 64) | StateId(bits_hash_);
  }

  static State MakeAllBits() {
    State t;
    for (int i = 0; i < kNumTargets; ++i) {
      t.SetBit(i);
    }
    t.bits_hash_ = absl::HashOf(*t.words_);
    return t;
  }

  static const State& AllBits() {
    static State ab = MakeAllBits();
    return ab;
  }

  friend State operator&(const State& lhs, const State& rhs) {
    State ans;
    ans.num_bits_ = 0;
    ans.min_bit_index_ = kNumTargets;
    ans.max_bit_index_ = 0;
    for (int i = 0; i < kNumWords; ++i) {
      uint64_t word = (*lhs.words_)[i] & (*rhs.words_)[i];
      (*ans.words_)[i] = word;
      ans.num_bits_ += __builtin_popcountll(word);
      if (word != 0) {
        ans.max_bit_index_ = (64 * i) + 63 - absl::countl_zero(word);
        if (ans.min_bit_index_ == kNumTargets) {
          ans.min_bit_index_ = (64 * i) + absl::countr_zero(word);
        }
      }
    }
    ans.bits_hash_ = absl::HashOf(*ans.words_);
    return ans;
  }

  const char* Exemplar() const {
    return (min_bit_index_ == kNumTargets) ? "NONE"
                                           : Word(min_bit_index_).ToString();
  }
  const char* Exemplar2() const { return Word(max_bit_index_).ToString(); }

  int count() const { return num_bits_; }
  bool empty() const { return num_bits_ == 0; }

  bool operator==(const State& r) const { return ToStateId() == r.ToStateId(); }
  bool operator!=(const State& r) const { return ToStateId() != r.ToStateId(); }
  bool operator<(const State& r) const { return ToStateId() < r.ToStateId(); }
  bool operator<=(const State& r) const { return ToStateId() <= r.ToStateId(); }
  bool operator>(const State& r) const { return ToStateId() > r.ToStateId(); }
  bool operator>=(const State& r) const { return ToStateId() >= r.ToStateId(); }

 private:
  State() : words_(std::make_unique<Array>()), num_bits_(0) {
    std::fill(words_->begin(), words_->end(), uint64_t{0});
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
    min_bit_index_ = std::min<uint16_t>(min_bit_index_, idx);
    max_bit_index_ = std::max<uint16_t>(max_bit_index_, idx);
  }

  static constexpr int kNumWords = (kNumTargets + 63) / 64;
  using Array = std::array<uint64_t, kNumWords>;
  std::unique_ptr<Array> words_;

  // We cheat here by assuming sequences with equal hashes are equal
  // (so long as the other three fields are equal too).
  //
  // This order of fields allows for fast StateId generation speed on x86.
  uint64_t bits_hash_ = 0;
  uint16_t max_bit_index_ = 0;
  uint16_t min_bit_index_ = kNumTargets;
  uint32_t num_bits_;
};

}  // namespace wordle