#pragma once

#include <iostream>
#include <string>
#include <string_view>

namespace wordle {

class Result {
 public:
  Result() : value_(0) {}
  Result(std::string_view sv);
  Result(const Result&) = default;
  Result(Result&&) = default;
  Result& operator=(const Result&) = default;
  Result& operator=(Result&&) = default;

  int Get(int i) const {
    int v = value_ >> (i * 2);
    return v % 4;
  }

  void Set(int idx, int v) {
    value_ &= ~(3 << (2 * idx));
    value_ |= v << (2 * idx);
  }

  friend std::ostream& operator<<(std::ostream& os, const Result& r) {
    for (int i = 0; i < 5; ++i) {
      os << r.Get(i);
    }
    return os;
  }

  bool operator==(const Result& r) const { return value_ == r.value_; }
  bool operator!=(const Result& r) const { return value_ != r.value_; }
  template <typename H>
  friend H AbslHashValue(H h, const Result& r) {
    return H::combine(std::move(h), r.value_);
  }

 private:
  uint16_t value_;
};

// Return a string representing the color output of a given guess vs a given
// target.  This is not fast, but it doesn't need to be.
//
// Both strings must be length five.  This is not checked.
Result Score(const char* guess, const char* target);

}  // namespace wordle