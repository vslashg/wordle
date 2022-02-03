#pragma once

#include <atomic>

#include "partition_map.h"
#include "state.h"

namespace wordle {

using CachedCallback = void (*)(const State& s, int score, const char* guess);

// A number which scores can never reach.  The initial state can achieve a
// score of 7920 through the guess `salet`.  (It's not yet known if this is
// best, but it does present an upper bound.)
constexpr int kScoreLimit = 7921;

// A number which a scoring function can use to indicate early termination via
// pruning.  Any result >= to this number represents no value.
constexpr int kOver = 100000;

// Calculate the score of the given state.  A score is the total number of
// guesses that would be required with optimal play if this state was entered
// once for every word still live in the given state.  (In particular, if `N`
// bits are set in `s`, then the return value can be devided by `N` to calculate
// the expected number of guesses needed from that position.
//
// `limit` represents a maximum usable score.  If `ScoreState` determines that
// the score is larger than this number, it will terminate early and return
// kOver.  (Note it is not guaranteed that this early exit will happen; this
// function can return values greater than kScoreLimit that are not kOver.)
int ScoreState(const State& s, int limit = kScoreLimit);

// Calculate the score of a given state `s`, presuming the word guess `p` is
// made.  Barring `kOver`/`limit` pruning, `ScoreState(s)` will return the
// minimum value of `ScoreState(s, p)` over all partitions `p` from state `s`.
//
// As with ScoreState, `limit` allows for early termination.
//
// Should not be called for states with 2 or fewer bits set.  ScoreState()
// will short circuit in this case.
int ScoreStatePartition(const State& s, const FullPartition& p, int limit);

// As above, but using a pointer to an atomic to use an updateable limit.
int ScoreStatePartition(const State& s, const FullPartition& p,
                        const std::atomic<int>* limit);

inline int ScoreStatePartition(const State& s, const FullPartition& p,
                               int limit) {
  std::atomic<int> a{limit};
  return ScoreStatePartition(s, p, &a);
}

// ScoreState but using a reduced map.
//
// XXXX s.count() <= 256
int PackedScoreState(const State& s, int limit = kScoreLimit);

}  // namespace wordle