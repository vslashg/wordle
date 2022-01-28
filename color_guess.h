#pragma once

#include <iostream>
#include <string>
#include <string_view>

#include "dictionary.h"

namespace wordle {

class Colors {
 public:
  Colors() : value_(0) {}
  Colors(std::string_view sv);
  Colors(const Colors&) = default;
  Colors(Colors&&) = default;
  Colors& operator=(const Colors&) = default;
  Colors& operator=(Colors&&) = default;

  int Get(int i) const {
    int v = value_ >> (i * 2);
    return v % 4;
  }

  void Set(int idx, int v) {
    value_ &= ~(3 << (2 * idx));
    value_ |= v << (2 * idx);
  }

  friend std::ostream& operator<<(std::ostream& os, const Colors& r) {
    for (int i = 0; i < 5; ++i) {
      os << r.Get(i);
    }
    return os;
  }

  bool operator==(const Colors& r) const { return value_ == r.value_; }
  bool operator!=(const Colors& r) const { return value_ != r.value_; }
  template <typename H>
  friend H AbslHashValue(H h, const Colors& r) {
    return H::combine(std::move(h), r.value_);
  }

 private:
  uint16_t value_;
};

// Return an object representing the color output of a given guess vs a given
// target.  This is not fast, but it doesn't need to be.
Colors ColorGuess(Word guess, Word target);

}  // namespace wordle