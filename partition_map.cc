#include "partition_map.h"

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

FullPartition MakeRootPartition(Word guess) {
  FullPartition p;
  p.word = guess;
  absl::flat_hash_map<Colors, std::bitset<kNumTargets>> parts;
  int tidx = 0;
  for (Word target : Word::AllTargetWords()) {
    if (guess != target) {
      Colors c = ColorGuess(guess, target);
      parts[c].set(tidx);
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

FullPartitionMap::FullPartitionMap() {
  for (Word w : Word::AllWords()) {
    all_partitions_.push_back(MakeRootPartition(w));
  }
  SortPartitions(all_partitions_);
}

std::vector<FullPartition> FullPartitionMap::SubPartitions(
    const State& in, bool sort_uniq) const {
  std::vector<FullPartition> result;
  for (const FullPartition& p : all_partitions_) {
    FullPartition filtered;
    filtered.word = p.word;
    for (const FullBranch& b : p.branches) {
      State mix = in & b.mask;
      int c = mix.count();
      if (c == in.count()) {
        if (!sort_uniq) {
          // only emit no-op move when sort_uniq off
          filtered.branches.push_back({b.colors, std::move(mix)});
        }
      } else if (c > 0) {
        filtered.branches.push_back({b.colors, std::move(mix)});
      }
    }
    if (!filtered.branches.empty()) {
      if (!sort_uniq) {
        // We don't want to uniquify answers in the mode, but we still want
        // the biggest chunks up front.
        SortPartition(filtered);
      }
      result.push_back(std::move(filtered));
    }
  }
  if (sort_uniq) {
    SortPartitions(result);
    result.erase(
        std::unique(result.begin(), result.end(), PartitionEq<State>{}),
        result.end());
  }
  return result;
};

}  // namespace wordle