#include "partition_map.h"
#include "raw_data.h"

#include "absl/container/flat_hash_map.h"

namespace wordle {

namespace {

template <typename MaskType>
struct BranchCmp {
  bool operator()(const Branch<MaskType>& lhs,
                  const Branch<MaskType>& rhs) const {
    return lhs.mask < rhs.mask;
  }
};

template <typename MaskType>
struct PartitionCmp {
  bool operator()(const Partition<MaskType>& lhs,
                  const Partition<MaskType>& rhs) const {
    return std::lexicographical_compare(
        lhs.branches.begin(), lhs.branches.end(), rhs.branches.begin(),
        rhs.branches.end(), BranchCmp<MaskType>{});
  }
};

template <typename MaskType>
struct PartitionEq {
  bool operator()(const Partition<MaskType>& lhs,
                  const Partition<MaskType>& rhs) const {
    const auto& lb = lhs.branches;
    const auto& rb = rhs.branches;
    return lb.size() == rb.size() &&
           std::equal(
               lb.begin(), lb.end(), rb.begin(),
               [](const auto& l, const auto& r) { return l.mask == r.mask; });
  }
};

void SortPartition(FullPartition& p) {
  std::sort(p.branches.begin(), p.branches.end(), BranchCmp<State>{});
  std::reverse(p.branches.begin(), p.branches.end());
}

void SortPartitions(std::vector<FullPartition>& ps) {
  for (FullPartition& p : ps) {
    SortPartition(p);
  }
  std::sort(ps.begin(), ps.end(), PartitionCmp<State>{});
}

}  // namespace

std::vector<FullPartition> SubPartitions(const State& in) {
  std::vector<FullPartition> result;
  for (const raw::Guess& guess : raw::guesses) {
    FullPartition filtered;
    filtered.word = guess.word;
    for (const raw::Indices& branch : guess.branches) {
      State mix(in, branch.Mask1(), branch.Mask2());
      int c = mix.count();
      if (c > 0 && c != in.count()) {
        filtered.branches.push_back({branch.colors, std::move(mix)});
      }
    }
    if (!filtered.branches.empty()) {
      result.push_back(std::move(filtered));
    }
  }
  SortPartitions(result);
  result.erase(std::unique(result.begin(), result.end(), PartitionEq<State>{}),
               result.end());
  return result;
};

}  // namespace wordle