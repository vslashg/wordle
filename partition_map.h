#pragma once

#include <vector>

#include "color_guess.h"
#include "state.h"

namespace wordle {

struct Branch {
  Branch() = default;
  Branch(const Branch&) = delete;
  Branch(Branch&&) = default;
  Branch& operator=(const Branch&) = delete;
  Branch& operator=(Branch&&) = default;

  Colors colors;
  State mask;
};

struct Partition {
  Partition() = default;
  Partition(const Partition&) = delete;
  Partition(Partition&&) = default;
  Partition& operator=(const Partition&) = delete;
  Partition& operator=(Partition&&) = default;

  const char* word;
  std::vector<Branch> branches;
};

class PartitionMap {
 public:
  static const PartitionMap& Singleton() {
    static PartitionMap pm;
    return pm;
  }

  const std::vector<Partition>& AllPartitions() const {
    return all_partitions_;
  }

  std::vector<Partition> SubPartitions(const State& input,
                                       bool sort_uniq = true) const;

 private:
  PartitionMap();

  std::vector<Partition> all_partitions_;
};

}  // namespace wordle