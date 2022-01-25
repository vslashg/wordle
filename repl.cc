#include <atomic>
#include <cstdio>
#include <iostream>
#include <thread>

#include "absl/time/clock.h"
#include "partition_map.h"
#include "score.h"

wordle::PartitionMap pm;

char depth[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

int dstep[20] = {0};
int dmax[20] = {0};
int dbest[20] = {9999};
int dbstep[20] = {0};
int dbmax[20] = {0};
int dbfree[20] = {0};
const char* dword[20] = {"?????", "?????", "?????", "?????", "?????"};

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

std::atomic<bool> go;

void MaybeIo() {
  if (go.load()) {
    printf(
        "%05d/%05d [%04d/%04d %04d] %s(%04d) %05d/%05d %s(%04d) %05d/%05d "
        "%05d/%05d\r",
        dstep[0], dmax[0], dbstep[0], dbmax[0], dbfree[0], dword[0], dbest[0],
        dstep[1], dmax[1], dword[1], dbest[1], dstep[2], dmax[2], dstep[3],
        dmax[3]);
    fflush(stdout);
    go.store(false);
  }
}

constexpr int kOver = 100000;

int BestScore(const wordle::State& s, int limit = 8185, int depth = 0) {
  int simple_limit = s.count() * 2 - 1;
  if (simple_limit >= limit) return kOver;
  if (s.count() < 3) return simple_limit;

  auto partitions = pm.SubPartitions(s);
  dmax[depth] = partitions.size();
  dbest[depth] = 9999;
  dword[depth] = "?????";

  int best_so_far = kOver;
  for (int i = 0; i < int(partitions.size()); ++i) {
    dstep[depth] = i;
    MaybeIo();
    const wordle::Partition& p = partitions[i];
    int score = s.count();
    for (const wordle::Branch& b : p.branches) {
      score += 2 * b.mask.count() - 1;
    }
    if (score >= limit) {
      continue;
    }
    dbmax[depth] = p.branches.size();
    int jj = 0;
    for (const wordle::Branch& b : p.branches) {
      dbstep[depth] = jj;
      ++jj;
      score -= 2 * b.mask.count() - 1;
      dbfree[depth] = limit - score;
      score += BestScore(b.mask, limit - score, depth + 1);
      if (score >= limit) break;
    }
    if (score < limit) {
      best_so_far = score;
      dbest[depth] = score;
      dword[depth] = p.word;
      limit = score;
    }
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