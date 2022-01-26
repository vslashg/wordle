#include <atomic>
#include <cstdio>
#include <iostream>
#include <thread>

#include "absl/time/clock.h"
#include "partition_map.h"
#include "score.h"
#include "thread_pool.h"
#include "folly/container/EvictingCacheMap.h"

wordle::PartitionMap pm;

absl::Mutex memomap_mu;

template<>
struct std::hash<wordle::StateId> {
  size_t operator()(wordle::StateId id) const {
    return id;
  }
};

folly::EvictingCacheMap<wordle::StateId, int>
    memomap(50'000'000);

int dstep[20] = {0};
int dmax[20] = {0};
int dbest[20] = {9999};

int dcached = 0;
int dnew = 0;
int dover = 0;
int dwasted = 0;
const char* dword[20] = {"?????", "?????", "?????", "?????", "?????"};

std::atomic<bool> go;

void MaybeIo() {
  if (go.load()) {
    printf(
        "%05d/%05d %s(%04d) ca=%-8d n=%-8d ov=%-8d ws=%-8d\r",
        dstep[0], dmax[0], dword[0], dbest[0], dcached, dnew, dover, dwasted);
    fflush(stdout);
    go.store(false);
  }
}

constexpr int kOver = 100000;

// A number for which a score can never reach
// the initial state scores 7997 via `soare`
constexpr int kScoreLimit = 7998;

int BestScore(const wordle::State& s, int limit = kScoreLimit, int depth = 0);

int ScorePartition(const wordle::State& s, const wordle::Partition& p,
                   int depth, std::atomic<int>* limit) {
  int score = s.count();
  for (const wordle::Branch& b : p.branches) {
    score += 2 * b.mask.count() - 1;
  }
  for (const wordle::Branch& b : p.branches) {
    if (score >= limit->load()) {
      // We've hit the limit, exit early
      return kOver;
    }
    score -= 2 * b.mask.count() - 1;
    score += BestScore(b.mask, limit->load() - score, depth + 1);
  }
  return score;
}

int BestScore(const wordle::State& s, int limit, int depth) {
  {
    absl::MutexLock lock(&memomap_mu);
    auto it = memomap.find(s.ToStateId());
    if (it != memomap.end()) {
      ++dcached;
      return it->second;
    }
  }
  MaybeIo();
  int simple_limit = s.count() * 2 - 1;
  if (simple_limit >= limit) return kOver;
  if (s.count() < 3) return simple_limit;

  auto partitions = pm.SubPartitions(s);
  dstep[depth] = 0;
  dmax[depth] = partitions.size();
  dbest[depth] = 9999;
  dword[depth] = "?????";

  int best_so_far = kOver;
  std::atomic<int> atomic_limit{limit};
  std::vector<std::function<int()>> choice_functions;
  for (int i = 0; i < int(partitions.size()); ++i) {
    choice_functions.emplace_back([&, i] {
      const wordle::Partition& p = partitions[i];
      int sc = ScorePartition(s, p, depth, &atomic_limit);
      if (sc < atomic_limit.load()) {
        atomic_limit.store(sc);
        dbest[depth] = sc;
        dword[depth] = p.word;
      }
      return sc;
    });
  }
  wordle::RunThreads(depth == 0 ? 32 : 1, choice_functions, [&](int score) {
    ++dstep[depth];
    best_so_far = std::min(best_so_far, score);
  });
  if (best_so_far < kOver) {
    absl::MutexLock lock(&memomap_mu);
    if (memomap.insert(s.ToStateId(), best_so_far).second) {
      ++dnew;
    } else {
      ++dwasted;
    }
  } else {
    ++dover;
  }
  return best_so_far;
}

void Plinko() {
  int i = 0;
  while (true) {
    go.store(true);
    ++i;
    if (i == 3000) {
      printf("\n");
      i = 0;
    }
    absl::SleepFor(absl::Milliseconds(100));
  }
}

int main() {
  std::thread t(Plinko);
  std::cout << "\n\n" << BestScore(wordle::State::AllBits()) << "\n\n";
}
