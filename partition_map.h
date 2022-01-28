#pragma once

#include <vector>

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

class FullPartitionMap {
 public:
  static const FullPartitionMap& Singleton() {
    static FullPartitionMap pm;
    return pm;
  }

  const std::vector<FullPartition>& AllPartitions() const {
    return all_partitions_;
  }

  std::vector<FullPartition> SubPartitions(const State& input,
                                           bool sort_uniq = true) const;

 private:
  FullPartitionMap();

  std::vector<FullPartition> all_partitions_;
};

}  // namespace wordle