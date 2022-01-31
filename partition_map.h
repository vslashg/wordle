#pragma once

#include <vector>

#include "absl/strings/str_format.h"
#include "color_guess.h"
#include "state.h"

namespace wordle {

template <typename MaskType>
struct Branch {
  Branch() = default;
  Branch(const Branch&) = delete;
  Branch(Branch&&) = default;
  Branch& operator=(const Branch&) = delete;
  Branch& operator=(Branch&&) = default;

  Colors colors;
  MaskType mask;
};

template <typename MaskType>
struct Partition {
  Partition() = default;
  Partition(const Partition&) = delete;
  Partition(Partition&&) = default;
  Partition& operator=(const Partition&) = delete;
  Partition& operator=(Partition&&) = default;

  using BranchType = Branch<MaskType>;
  Word word;
  std::vector<Branch<MaskType>> branches;
};

using FullPartition = Partition<State>;
using FullBranch = Branch<State>;

std::vector<FullPartition> SubPartitions(const State& input);

}  // namespace wordle