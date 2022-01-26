#include <string_view>

#include "color_guess.h"

namespace wordle {

Colors::Colors(std::string_view s) : value_(0) {
  for (size_t i = 0; i < 5; ++i) {
    if (s.size() <= i) return;
    if (s[i] == '1') Set(i, 1);
    if (s[i] == '2') Set(i, 2);
  }
}

Colors ColorGuess(const char* guess, const char* target) {
  std::string g = guess;
  std::string t = target;
  Colors ans;
  // exact matches
  for (int i = 0; i < 5; ++i) {
    if (g[i] == t[i]) {
      g[i] = ' ';
      t[i] = '_';
      ans.Set(i, 2);
    }
  }
  for (int i = 0; i < 5; ++i) {
    auto pos = t.find(g[i]);
    if (pos != std::string::npos) {
      t[pos] = '_';
      ans.Set(i, 1);
    }
  }
  return ans;
}

}  // namespace wordle
