#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace wordle {

constexpr int kNumTargets = 2309;
constexpr int kNumNonTargets = 10638;
constexpr int kDictionarySize = kNumTargets + kNumNonTargets;

class Word {
 public:
  Word() : index_(0xffff) {}

  // Constructs the given word.  If this word does not exist in the dictionary,
  // this constructs the sentinel word ????? instead.
  Word(std::string_view s);
  constexpr Word(int i) : index_(i) {}

  Word(const Word&) = default;
  Word& operator=(const Word&) = default;

  // Returns a list of every word in the dictionary.
  static const std::vector<Word>& AllWords();
  // Returns a list of target words.
  static const std::vector<Word>& AllTargetWords();

  // Returns true if this word is in the target dictionary.
  bool IsTarget() const { return index_ <= kNumTargets; }

  bool IsSentinel() const { return index_ == 0xffff; }
  bool IsValid() const { return !IsSentinel(); }

  const char* ToString() const;
  int ToIndex() const { return index_; }

  friend std::ostream& operator<<(std::ostream& os, const Word& w) {
    return os << w.ToString();
  }

  friend std::istream& operator>>(std::istream& is, Word& w) {
    std::string s;
    is >> s;
    w = Word(s);
    return is;
  }

  const bool operator==(Word& rhs) const { return index_ == rhs.index_; }
  const bool operator!=(Word& rhs) const { return index_ != rhs.index_; }
  const bool operator<(Word& rhs) const { return index_ < rhs.index_; }
  const bool operator<=(Word& rhs) const { return index_ <= rhs.index_; }
  const bool operator>(Word& rhs) const { return index_ > rhs.index_; }
  const bool operator>=(Word& rhs) const { return index_ >= rhs.index_; }

 private:
  uint16_t index_;
};

}  // namespace wordle