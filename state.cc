#include "state.h"

namespace wordle {

std::vector<Word> State::Words() const {
  std::vector<Word> result;
  for (int i = 0; i < kNumWords; ++i) {
    uint64_t word = (*words_)[i];
    while (word) {
      int bit_index = absl::countr_zero(word);
      result.emplace_back(i * 64 + bit_index);
      word ^= (uint64_t{1} << bit_index);
    }
  }
  return result;
}

}  // namespace wordle