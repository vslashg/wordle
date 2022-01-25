#pragma once

#include <vector>

#include "score.h"
#include "state.h"

namespace wordle {

struct Branch {
  Branch() = default;
  Branch(const Branch&) = delete;
  Branch(Branch&&) = default;
  Branch& operator=(const Branch&) = delete;
  Branch& operator=(Branch&&) = default;

  Result result;
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
  PartitionMap();

  const std::vector<Partition>& AllPartitions() const {
    return all_partitions_;
  }

  std::vector<Partition> SubPartitions(const State& input) const;

 private:
  std::vector<Partition> all_partitions_;
};

}  // namespace wordle