#include "score.h"

#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "reduced_map.h"

namespace wordle {

namespace {

absl::Mutex big_results_mu;
absl::flat_hash_map<uint64_t, ScoreResult> big_results;

}  // namespace

bool AddHash(uint64_t rapidash, ScoreResult res) {
  absl::MutexLock lock(&big_results_mu);
  return big_results.emplace(rapidash, res).second;
}

bool IsCached(const State& s) {
  uint64_t rapidash = s.Rapidash();
  absl::MutexLock lock(&big_results_mu);
  return big_results.contains(rapidash);
}

int ScoreStatePartition(const State& s, const FullPartition& p, int limit) {
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
    if (score >= limit) {
      // We've hit the limit, exit early
      return kOver;
    }
    // Subtract out our lower bound guess for this branch.
    score -= 2 * b.mask.count() - 1;
    // Recursively call BestScore, subtracting out our score so far from the
    // limit that we pass to the child.
    score += ScoreState(b.mask, limit - score).first;
  }
  return score;
}

namespace {

template <int N>
ScoreResult PackedScoreState(
    const ReducedPartitions<N>& rpm,
    absl::flat_hash_map<std::array<uint64_t, N>, ScoreResult>& cache,
    const std::array<uint64_t, N>& s, int count, int limit);

template <int N>
int PackedScoreStatePartition(
    const ReducedPartitions<N>& rpm,
    absl::flat_hash_map<std::array<uint64_t, N>, ScoreResult>& cache,
    const std::array<uint64_t, N>& s, int count, const ReducedGuess<N>& p,
    int limit) {
  // The base score is one for each bit in `s`, indicating the
  // guess we're about to make.
  int score = count;

  // Each branch in this partition represents a different set of colors for
  // our guess.  We must increase the score by the score of each such state
  // we are transitioning to.

  // To enable early pruning, we first add in a lower bound value for each
  // match.  (A state with N bits has as a lower bound 2N-1 as a score.)
  std::vector<const wordle::ReducedBranch<N>*> branches_left;
  for (const wordle::ReducedBranch<N>& b : p.branches) {
    auto it = cache.find(b.mask);
    if (it == cache.end()) {
      score += 2 * b.num_bits - 1;
      branches_left.push_back(&b);
    } else {
      score += it->second.first;
    }
  }
  if (score >= limit) {
    // We've hit the limit, exit early
    return kOver;
  }
  for (const wordle::ReducedBranch<N>* b : branches_left) {
    // Subtract out our lower bound guess for this branch.
    score -= 2 * b->num_bits - 1;
    // Recursively call BestScore, subtracting out our score so far from the
    // limit that we pass to the child.
    score +=
        PackedScoreState<N>(rpm, cache, b->mask, b->num_bits, limit - score)
            .first;
    if (score >= limit) {
      // We've hit the limit, exit early
      return kOver;
    }
  }
  return score;
}

template <int N>
ScoreResult PackedScoreState(
    const ReducedPartitions<N>& rpm,
    absl::flat_hash_map<std::array<uint64_t, N>, ScoreResult>& cache,
    const std::array<uint64_t, N>& s, int count, int limit) {
  int simple_limit = count * 2 - 1;
  if (simple_limit >= limit) return {kOver, Word{}};
  if (count < 3) return ScoreResult{simple_limit, rpm.Exemplar(s)};

  auto it = cache.find(s);
  if (it != cache.end()) {
    return it->second;
  }

  std::vector<ReducedGuess<N>> partitions = rpm.SubPartitions(s);
  ScoreResult best_so_far = {kOver, Word()};
  for (const ReducedGuess<N>& p : partitions) {
    int sc = PackedScoreStatePartition<N>(rpm, cache, s, count, p, limit);
    if (sc < limit) {
      limit = sc;
      best_so_far = {sc, p.word};
    }
  }
  if (best_so_far.first < kOver) {
    cache[s] = best_so_far;
  }
  return best_so_far;
}

ScoreResult PackedScoreState(const State& s, int limit) {
  int count = s.count();
  if (count <= 64 * 1) {
    ReducedPartitions<1> rpm(s);
    absl::flat_hash_map<std::array<uint64_t, 1>, ScoreResult> cache;
    return PackedScoreState<1>(rpm, cache, rpm.FullMask(), count, limit);
  } else if (count <= 64 * 2) {
    ReducedPartitions<2> rpm(s);
    absl::flat_hash_map<std::array<uint64_t, 2>, ScoreResult> cache;
    return PackedScoreState<2>(rpm, cache, rpm.FullMask(), count, limit);
  } else if (count <= 64 * 3) {
    ReducedPartitions<3> rpm(s);
    absl::flat_hash_map<std::array<uint64_t, 3>, ScoreResult> cache;
    return PackedScoreState<3>(rpm, cache, rpm.FullMask(), count, limit);
  } else if (count <= 64 * 4) {
    ReducedPartitions<4> rpm(s);
    absl::flat_hash_map<std::array<uint64_t, 4>, ScoreResult> cache;
    return PackedScoreState<4>(rpm, cache, rpm.FullMask(), count, limit);
  } else {
    return ScoreState(s, limit);
  }
}

}  // namespace

ScoreResult ScoreState(const State& s, int limit) {
  int simple_limit = s.count() * 2 - 1;
  if (simple_limit >= limit) return {kOver, Word{}};
  if (s.count() < 3) return ScoreResult{simple_limit, s.Exemplar()};
  uint64_t bighash;
  if (s.count() >= kCutoff) {
    bighash = s.Rapidash();
    absl::MutexLock lock(&big_results_mu);
    auto it = big_results.find(bighash);
    if (it != big_results.end()) {
      return it->second;
    }
  }
  if (s.count() < 257) {
    return PackedScoreState(s, limit);
  }

  std::vector<FullPartition> partitions = SubPartitions(s);
  ScoreResult best_so_far = {kOver, Word{}};
  for (const FullPartition& p : partitions) {
    int sc = ScoreStatePartition(s, p, limit);
    if (sc < limit) {
      limit = sc;
      best_so_far = {sc, p.word};
    }
  }
  return best_so_far;
}

}  // namespace wordle