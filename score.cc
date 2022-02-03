#include "score.h"

#include "reduced_map.h"

namespace wordle {

int ScoreStatePartition(const State& s, const FullPartition& p,
                        const std::atomic<int>* limit) {
  // The base score is one for each bit in `s`, indicating the
  // guess we're about to make.
  int score = s.count();

  // Each branch in this partition represents a different set of colors for
  // our guess.  We must increase the score by the score of each such state
  // we are transitioning to.

  // To enable early pruning, we first add in a lower bound value for each
  // match.  (A state with N bits has as a lower bound 2N-1 as a score.)
  for (const FullPartition::BranchType& b : p.branches) {
    score += 2 * b.mask.count() - 1;
  }
  for (const FullPartition::BranchType& b : p.branches) {
    if (score >= limit->load()) {
      // We've hit the limit, exit early
      return kOver;
    }
    // Subtract out our lower bound guess for this branch.
    score -= 2 * b.mask.count() - 1;
    // Recursively call BestScore, subtracting out our score so far from the
    // limit that we pass to the child.
    score += ScoreState(b.mask, limit->load() - score);
  }
  return score;
}

int ScoreState(const State& s, int limit, int num_threads) {
  int simple_limit = s.count() * 2 - 1;
  if (simple_limit >= limit) return kOver;
  if (s.count() < 3) return simple_limit;

  std::vector<FullPartition> partitions = SubPartitions(s);
  int best_so_far = kOver;
  for (const FullPartition& p : partitions) {
    int sc = ScoreStatePartition(s, p, limit);
    if (sc < limit) {
      limit = sc;
      best_so_far = sc;
    }
  }
  return best_so_far;
}

namespace {

template <int N>
int PackedScoreState(const ReducedPartitions<N>& rpm,
                     const std::array<uint64_t, N>& s, int count, int limit);

template <int N>
int PackedScoreStatePartition(const ReducedPartitions<N>& rpm,
                              const std::array<uint64_t, N>& s,
                              int count,
                              const ReducedGuess<N>& p, int limit) {
  // The base score is one for each bit in `s`, indicating the
  // guess we're about to make.
  int score = count;

  // Each branch in this partition represents a different set of colors for
  // our guess.  We must increase the score by the score of each such state
  // we are transitioning to.

  // To enable early pruning, we first add in a lower bound value for each
  // match.  (A state with N bits has as a lower bound 2N-1 as a score.)
  for (const wordle::ReducedBranch<N>& b : p.branches) {
    score += 2 * b.num_bits - 1;
  }
  for (const wordle::ReducedBranch<N>& b : p.branches) {
    if (score >= limit) {
      // We've hit the limit, exit early
      return kOver;
    }
    // Subtract out our lower bound guess for this branch.
    score -= 2 * b.num_bits - 1;
    // Recursively call BestScore, subtracting out our score so far from the
    // limit that we pass to the child.
    score += PackedScoreState<N>(rpm, b.mask, b.num_bits, limit - score);
  }
  return score;
}

template <int N>
int PackedScoreState(const ReducedPartitions<N>& rpm,
                     const std::array<uint64_t, N>& s, int count, int limit) {
  int simple_limit = count * 2 - 1;
  if (simple_limit >= limit) return kOver;
  if (count < 3) return simple_limit;

  std::vector<ReducedGuess<N>> partitions = rpm.SubPartitions(s);
  int best_so_far = kOver;
  for (const ReducedGuess<N>& p : partitions) {
    int sc = PackedScoreStatePartition<N>(rpm, s, count, p, limit);
    if (sc < limit) {
      limit = sc;
      best_so_far = sc;
    }
  }
  return best_so_far;
}

}  // namespace

int PackedScoreState(const State& s, int limit) {
  int count = s.count();
  if (count <= 64 * 1) {
    ReducedPartitions<1> rpm(s);
    return PackedScoreState<1>(rpm, rpm.FullMask(), count, limit);
  } else if (count <= 64 * 2) {
    ReducedPartitions<2> rpm(s);
    return PackedScoreState<2>(rpm, rpm.FullMask(), count, limit);
  } else if (count <= 64 * 3) {
    ReducedPartitions<3> rpm(s);
    return PackedScoreState<3>(rpm, rpm.FullMask(), count, limit);
  } else if (count <= 64 * 4) {
    ReducedPartitions<4> rpm(s);
    return PackedScoreState<4>(rpm, rpm.FullMask(), count, limit);
  } else {
    return ScoreState(s, limit);
  }
}

}  // namespace wordle