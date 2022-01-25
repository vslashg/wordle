#include <atomic>
#include <cstdio>
#include <iostream>
#include <thread>

#include "absl/time/clock.h"
#include "partition_map.h"
#include "score.h"
#include "thread_pool.h"

wordle::PartitionMap pm;

int dstep[20] = {0};
int dmax[20] = {0};
int dbest[20] = {9999};
const char* dword[20] = {"?????", "?????", "?????", "?????", "?????"};
/*
int Score(const wordle::State& s, char* d = depth) {
  *d = '-';
  if (s.count() == 1) return 1;
  if (s.count() == 2) return 3;
  int score = s.count();
  wordle::Partition next = std::move(pm.SubPartitions(s).front());
  for (const wordle::Branch& b : next.branches) {
    score += Score(b.mask, d + 1);
  }
  return score;
}
*/
std::atomic<bool> go;

void MaybeIo() {
  if (go.load()) {
    printf(
        "%05d/%05d %s(%04d) %05d/%05d %s(%04d) %05d/%05d "
        "%05d/%05d\r",
        dstep[0], dmax[0], dword[0], dbest[0], dstep[1], dmax[1], dword[1],
        dbest[1], dstep[2], dmax[2], dstep[3], dmax[3]);
    fflush(stdout);
    go.store(false);
  }
}

constexpr int kOver = 100000;

// A number for which a score can never go above
constexpr int kScoreLimit = 8020;

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
