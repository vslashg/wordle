#include "partition_map.h"

#include "absl/container/flat_hash_map.h"

namespace wordle {

namespace {

struct BranchCmp {
  bool operator()(const Branch& lhs, const Branch& rhs) const {
    return std::make_tuple(lhs.mask.count(), lhs.mask.exemplar_index()) <
           std::make_tuple(rhs.mask.count(), rhs.mask.exemplar_index());
  }
};

struct PartitionCmp {
  bool operator()(const Partition& lhs, const Partition& rhs) const {
    return std::lexicographical_compare(
        lhs.branches.begin(), lhs.branches.end(), rhs.branches.begin(),
        rhs.branches.end(), BranchCmp{});
  }
};

struct PartitionEq {
  bool operator()(const Partition& lhs, const Partition& rhs) const {
    const auto& lb = lhs.branches;
    const auto& rb = rhs.branches;
    return lb.size() == rb.size() &&
           std::equal(lb.begin(), lb.end(), rb.begin(),
                      [](const auto& l, const auto& r) {
                        return l.mask.count() == r.mask.count() &&
                               l.mask.exemplar_index() ==
                                   r.mask.exemplar_index();
                      }) &&
           std::equal(
               lb.begin(), lb.end(), rb.begin(),
               [](const auto& l, const auto& r) { return l.mask == r.mask; });
  }
};

void SortPartition(Partition& p) {
  std::sort(p.branches.begin(), p.branches.end(), BranchCmp{});
  std::reverse(p.branches.begin(), p.branches.end());
}

void SortPartitions(std::vector<Partition>& ps) {
  for (Partition& p : ps) {
    SortPartition(p);
  }
  std::sort(ps.begin(), ps.end(), PartitionCmp{});
}

Partition MakeRootPartition(const char* guess) {
  Partition p;
  p.word = guess;
  absl::flat_hash_map<Result, std::bitset<kNumTargets>> parts;
  int tidx = 0;
  for (const char* target : targets) {
    if (guess != target) {
      Result r = Score(guess, target);
      parts[r].set(tidx);
    }
    ++tidx;
  }

  for (auto& pair : parts) {
    p.branches.push_back({pair.first, State(pair.second)});
  }
  SortPartition(p);
  return p;
}

}  // namespace

PartitionMap::PartitionMap() {
  for (const char* guess : targets) {
    all_partitions_.push_back(MakeRootPartition(guess));
  }
  std::cout << "\n";
  for (const char* guess : non_targets) {
    all_partitions_.push_back(MakeRootPartition(guess));
  }
  std::cout << "\n";
  SortPartitions(all_partitions_);
}

std::vector<Partition> PartitionMap::SubPartitions(const State& in) const {
  std::vector<Partition> result;
  for (const Partition& p : all_partitions_) {
    Partition filtered;
    filtered.word = p.word;
    for (const Branch& b : p.branches) {
      State mix = in & b.mask;
      int c = mix.count();
      if (c == in.count()) {
        break;  // no-op move
      } else if (c > 0) {
        filtered.branches.push_back({b.result, std::move(mix)});
      }
    }
    if (!filtered.branches.empty()) {
      result.push_back(std::move(filtered));
    }
  }
  SortPartitions(result);
  result.erase(std::unique(result.begin(), result.end(), PartitionEq{}),
               result.end());
  return result;
};

}  // namespace wordle