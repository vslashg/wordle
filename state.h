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
  static constexpr int kNumWords = (kNumTargets + 63) / 64;

  State(const std::bitset<kNumTargets>& b) : State() {
    for (int i = 0; i < kNumTargets; ++i) {
      if (b[i]) {
        SetBit(i);
      }
    }
    bits_hash_ = absl::HashOf(*words_);
  }

  State(const State& initial, const std::array<uint64_t, kNumWords>& m1,
        const std::array<uint64_t, kNumWords>& m2)
      : words_(std::make_unique<Array>()),
        max_bit_index_(0),
        min_bit_index_(kNumTargets),
        num_bits_(0) {
    for (int i = 0; i < kNumWords; ++i) {
      uint64_t word = (*initial.words_)[i] & m1[i] & m2[i];
      (*words_)[i] = word;
      num_bits_ += __builtin_popcountll(word);
      if (word != 0) {
        max_bit_index_ = (64 * i) + 63 - absl::countl_zero(word);
        if (min_bit_index_ == kNumTargets) {
          min_bit_index_ = (64 * i) + absl::countr_zero(word);
        }
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

  template <typename H>
  friend H AbslHashValue(H h, const State& s) {
    return H::combine(std::move(h), s.array());
  }

  uint64_t Rapidash() const {
    uint64_t sh = 14695981039346656037u;
    for (uint64_t word : *words_) {
      sh *= uint64_t{1099511628211};
      sh ^= (word >> 32);
      sh *= uint64_t{1099511628211};
      sh ^= (word & uint64_t{0xffffffff});
    }
    return sh;
  }

  static State MakeAllBits() {
    State t;
    for (int i = 0; i < kNumTargets; ++i) {
      t.SetBit(i);
    }
    t.bits_hash_ = absl::HashOf(*t.words_);
    return t;
  }

  static State MakeEmpty() {
    State t;
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
  std::vector<Word> Words() const;

  int count() const { return num_bits_; }
  bool empty() const { return num_bits_ == 0; }

  bool operator==(const State& r) const { return ToStateId() == r.ToStateId(); }
  bool operator!=(const State& r) const { return ToStateId() != r.ToStateId(); }
  bool operator<(const State& r) const { return ToStateId() < r.ToStateId(); }
  bool operator<=(const State& r) const { return ToStateId() <= r.ToStateId(); }
  bool operator>(const State& r) const { return ToStateId() > r.ToStateId(); }
  bool operator>=(const State& r) const { return ToStateId() >= r.ToStateId(); }

  using Array = std::array<uint64_t, kNumWords>;
  const Array& array() const { return *words_; }

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

// A thin state is like State, but with its bitmask collapsed to a much smaller
// set of legal words.
template <int N>
class ThinState {
 public:
  ThinState() {
    std::fill(words_.begin(), words_.end(), uint64_t{0});
  }

  ThinState(const ThinState&) = default;
  ThinState& operator=(const ThinState&) = default;

  friend ThinState operator&(const ThinState& l, const ThinState& r) {
    ThinState ts(Uninitialized{});
    for (int i = 0; i < N; ++i) {
      ts.words_[i] = l.words_[i] & r.words_[i];
    }
    return ts;
  }

  bool empty() const {
    for (uint64_t word : words_) {
      if (word) return false;
    }
    return true;
  }

  int count() const {
    int ans = 0;
    for (uint64_t word : words_) {
      ans += absl::popcount(word);
    }
    return ans;
  }

 private:
  struct Uninitialized {};
  ThinState(Uninitialized) {};

  std::array<uint64_t, N> words_;
};

}  // namespace wordle