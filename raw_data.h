#pragma once

#include <array>
#include <cstdint>

#include "color_guess.h"
#include "dictionary.h"
#include "absl/types/span.h"

namespace raw {

extern const std::array<uint64_t, 37> vowel_masks[13912];
extern const std::array<uint64_t, 37> consonant_masks[36237];

struct Indices {
  wordle::Colors colors;
  uint16_t vowel_index;
  uint16_t consonant_index;

  const std::array<uint64_t, 37>& Mask1() const {
    return vowel_masks[vowel_index];
  }
  const std::array<uint64_t, 37>& Mask2() const {
    return consonant_masks[consonant_index];
  }
};

struct Guess {
  constexpr Guess(int word, const Indices* ptr, int len)
      : w(word), branches(ptr, len) {}
  wordle::Word w;
  absl::Span<const Indices> branches;
};

extern const Guess guesses[12972];

}  // namespace raw