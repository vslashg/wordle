#include "score.h"

namespace wordle {

const FullPartitionMap& pm = FullPartitionMap::Singleton();

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

  std::vector<FullPartition> partitions = pm.SubPartitions(s);
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

}  // namespace wordle