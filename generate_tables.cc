#include <iostream>
#include <set>
#include <string>

#include "absl/strings/str_format.h"
#include "color_guess.h"
#include "dictionary.h"
#include "state.h"

using namespace wordle;

class StateIndexer {
 public:
  StateIndexer() {
    index_to_mask_.emplace_back(State::MakeEmpty());
    index_to_mask_.emplace_back(State::MakeAllBits());
    state_id_to_index_[index_to_mask_[0].ToStateId()] = 0;
    state_id_to_index_[index_to_mask_[1].ToStateId()] = 1;
  }

  int Index(const std::bitset<kNumTargets>& bitmask) {
    State s(bitmask);
    auto it = state_id_to_index_.find(s.ToStateId());
    if (it != state_id_to_index_.end()) {
      return it->second;
    }
    int new_index = index_to_mask_.size();
    state_id_to_index_[s.ToStateId()] = new_index;
    index_to_mask_.push_back(std::move(s));
    return new_index;
  }

  const State& Lookup(int index) const { return index_to_mask_[index]; }

  int size() const { return index_to_mask_.size(); }

 private:
  std::map<StateId, int> state_id_to_index_;
  std::deque<State> index_to_mask_;
};

std::set<std::pair<int, int>> g_all_indices;

struct Branch {
  Colors colors;
  int vowel_index;
  int consonant_index;
  int bit_count;
};

struct Guess {
  Word word;
  std::vector<Branch> branches;
};

std::vector<Branch> ScoreWord(Word guess, StateIndexer& vowel_indexer,
                              StateIndexer& consonant_indexer) {
  char vowel_guess[] = "*****";
  char consonant_guess[] = "*****";
  const char* full_guess = guess.ToString();
  for (int i = 0; i < 5; ++i) {
    char letter = full_guess[i];
    if (letter == 'a' || letter == 'e' || letter == 'i' || letter == 'o' ||
        letter == 'u' || letter == 'y' || letter == 's') {
      vowel_guess[i] = letter;
    } else {
      consonant_guess[i] = letter;
    }
  }
  std::map<Colors, std::bitset<kNumTargets>> vowel_masks;
  std::map<Colors, std::bitset<kNumTargets>> consonant_masks;
  std::map<Colors, std::pair<Colors, Colors>> splits;
  int idx = 0;
  for (Word target : Word::AllTargetWords()) {
    const char* full_target = target.ToString();
    Colors full_colors = ColorGuess(guess, target);
    Colors vowel_colors = ColorGuess(vowel_guess, full_target);
    Colors consonant_colors = ColorGuess(consonant_guess, full_target);
    splits[full_colors] = std::make_pair(vowel_colors, consonant_colors);
    vowel_masks[vowel_colors].set(idx);
    consonant_masks[consonant_colors].set(idx);
    ++idx;
  }

  std::map<Colors, int> vowel_indices;
  for (const auto& node : vowel_masks) {
    vowel_indices[node.first] = vowel_indexer.Index(node.second);
  }
  std::map<Colors, int> consonant_indices;
  for (const auto& node : consonant_masks) {
    consonant_indices[node.first] = consonant_indexer.Index(node.second);
  }
  std::vector<Branch> branches;
  for (const auto& node : splits) {
    Branch br;
    br.colors = node.first;
    br.vowel_index = vowel_indices[node.second.first];
    br.consonant_index = consonant_indices[node.second.second];
    State mask = vowel_indexer.Lookup(br.vowel_index) &
                 consonant_indexer.Lookup(br.consonant_index);
    br.bit_count = mask.count();
    branches.push_back(br);
    g_all_indices.insert(std::make_pair(br.vowel_index, br.consonant_index));
  }
  std::stable_sort(branches.begin(), branches.end(),
                   [](const Branch& lhs, const Branch& rhs) {
                     return lhs.bit_count > rhs.bit_count;
                   });
  return branches;
}

void emit_table(std::string_view name, const StateIndexer& indexer) {
  absl::PrintF(
      "  constexpr std::array<uint64_t, %d> %s[%d] = {\n   ",
      State::kNumWords, name, indexer.size());
  for (int i = 0; i < indexer.size(); ++i) {
    absl::PrintF(" {{  // %d\n      ", i);
    for (int j = 0; j < State::kNumWords; ++j) {
      int precision = (j != State::kNumWords - 1) ? 16 : 3;
      absl::PrintF("0x%0*x, ", precision, indexer.Lookup(i).array()[j]);
      if (j % 3 == 2 && j != State::kNumWords - 2) {
        absl::PrintF("\n      ");
      }
    }
    absl::PrintF("\n    }},");
  }
  absl::PrintF("\n  };\n\n");
}

void make_tables() {
  StateIndexer vowel_indexer;
  StateIndexer consonant_indexer;
  std::vector<Guess> guesses;
  for (Word w : Word::AllWords()) {
    Guess g;
    g.word = w;
    g.branches = ScoreWord(w, vowel_indexer, consonant_indexer);
    guesses.push_back(g);
  }
  std::stable_sort(
      guesses.begin(), guesses.end(), [](const Guess& lhs, const Guess& rhs) {
        return lhs.branches.front().bit_count < rhs.branches.front().bit_count;
      });

  std::set<StateId> ids;
  for (auto p : g_all_indices) {
    ids.insert(
        (vowel_indexer.Lookup(p.first) & consonant_indexer.Lookup(p.second))
            .ToStateId());
  }

  emit_table("vowel_masks", vowel_indexer);
  emit_table("consonant_masks", consonant_indexer);

  std::vector<Branch> all_branches;
  struct LocalGuess {
    Word word;
    uint32_t offset;
    uint32_t size;
  };
  std::vector<LocalGuess> local_guesses;

  for (const Guess& guess : guesses) {
    LocalGuess local_guess;
    local_guess.word = guess.word;
    local_guess.offset = all_branches.size();
    local_guess.size = guess.branches.size();
    all_branches.insert(all_branches.end(), guess.branches.begin(),
                        guess.branches.end());
    local_guesses.push_back(local_guess);
  }

  absl::PrintF(
      "  constexpr Indices mask_pairs[%d] = {\n   ",
      all_branches.size());
  for (int i = 0; i < int(all_branches.size()); ++i) {
    absl::PrintF(" %4d, %5d, %5d,", all_branches[i].colors.ToInt(),
                 all_branches[i].vowel_index, all_branches[i].consonant_index);
    if (i % 3 == 2) {
      absl::PrintF("\n   ");
    }
  }
  absl::PrintF("  };\n\n");

  absl::PrintF("  constexpr Guess guesses[%d] = {\n", local_guesses.size());
  for (const LocalGuess& local_guess : local_guesses) {
    absl::PrintF("    Guess(%5d, mask_pairs + %7d, %3d),  // %s\n",
                 local_guess.word.ToIndex(), local_guess.offset,
                 local_guess.size, local_guess.word.ToString());
  }
  absl::PrintF("  };\n\n");
}

int main(int argc, char** argv) {
  absl::PrintF(
      "#include \"raw_data.h\"\n"
      "#include <array>\n"
      "#include <cstdint>\n"
      "\n"
      "namespace raw {\n");
  make_tables();
  absl::PrintF("}  // namespace raw\n");
}