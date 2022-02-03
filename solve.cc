#include <iostream>
#include <string>
#include <string_view>

#include "partition_map.h"
#include "reduced_map.h"
#include "score.h"
#include "absl/time/time.h"
#include "absl/time/clock.h"

using namespace wordle;

template <int count>
void TimeTest(const State& mask) {
  if (mask.count() <= count * 64) {
    std::cout << "timing masks for " << count << " words\n";
    auto time1 = absl::Now();
    ReducedPartitions<4> rp(mask);
    auto time2 = absl::Now();
    std::cout << (time2 - time1) / absl::Microseconds(1) << "us\n";
    auto ans = rp.SubPartitions(rp.FullMask());
    auto time3 = absl::Now();
    std::cout << (time3 - time2) / absl::Microseconds(1) << "us\n";
    std::cout << ans.size() << " reduced branches after mask\n";
  }
}

constexpr bool branch = true;

int Count(const std::array<uint64_t, 4>& ary) {
  int c = 0;
  for (uint64_t word : ary) {
    c += absl::popcount(word);
  }
  return c;
}

int Nice(wordle::State full_state) {
  std::cout << "Nice!\n";
  ReducedPartitions<4> rpm(full_state);
  std::array<uint64_t, 4> state = rpm.FullMask();
  while (true) {
    int count = Count(state);
    if (count == 0) {
      return 0;
    }
    std::cout << count << " left, exemplar " << rpm.Exemplar(state) << "\n";
    if (count < 2) {
      return 0;
    }
    std::vector<wordle::ReducedGuess<4>> rgs = rpm.SubPartitions(state);
    if (rgs.empty()) {
      return 1;
    }
    std::cout << "I guess: " << rgs.front().word << "\n";
    std::cout << "I expect worst case is "
              << rgs.front().branches.front().num_bits << "\n";
    std::string colors;
    std::cout << "Colors? ";
    std::cin >> colors;
    Colors c(colors);
    for (const wordle::ReducedBranch<4>& b : rgs.front().branches) {
      if (b.colors == c) {
        state = b.mask;
        break;
      }
    }
  }
}

int main(int argc, char** argv) {
  wordle::State state = wordle::State::MakeAllBits();
  while (state.count() > 1) {
    if (argc == 1 && state.count() < 257) {
      return Nice(std::move(state));
    }
    auto sp = SubPartitions(state);
    if (sp.empty()) {
      break;
    }
    std::cout << " Best guess: " << sp.front().word << ", worst case is "
              << sp.front().branches.front().mask.count() << "\n";
    std::cout << "Worst guess: " << sp.back().word << ", worst case is "
              << sp.back().branches.front().mask.count() << "\n";
    Word guess;
    FullPartition* part = nullptr;
    while (!part) {
      std::string textguess;
      std::cout << "Guess? ";
      std::cin >> textguess;
      Word w2(textguess);
      for (FullPartition& p : sp) {
        if (p.word == w2) {
          part = &p;
        }
      }
    }
    std::string colors;
    std::cout << "Colors? ";
    std::cin >> colors;
    Colors c(colors);
    for (FullBranch& b : part->branches) {
      if (b.colors == c) {
        std::cout << b.mask.count() << " left, exemplar " << b.mask.Exemplar()
                  << "\n";
        TimeTest<6>(b.mask);
        TimeTest<5>(b.mask);
        TimeTest<4>(b.mask);
        TimeTest<3>(b.mask);
        TimeTest<2>(b.mask);
        TimeTest<1>(b.mask);
        if (branch && b.mask.count() < 256) {
          {
            auto time1 = absl::Now();
            int score = PackedScoreState(b.mask);
            auto time2 = absl::Now();
            std::cout << "PACKED Score " << score << ", EV "
                      << (double(score) / b.mask.count()) << ", time "
                      << (time2 - time1) / absl::Milliseconds(1) << "ms\n";
          }
          {
            auto time1 = absl::Now();
            int score = ScoreState(b.mask);
            auto time2 = absl::Now();
            std::cout << "FULL Score " << score << ", EV "
                      << (double(score) / b.mask.count()) << ", time "
                      << (time2 - time1) / absl::Milliseconds(1) << "ms\n";
          }
/*
          ReducedPartitions<4> rpm(b.mask);
          auto j = rpm.SubPartitions(rpm.FullMask());
          if (!j.empty()) {
            std::cout << "Reduced table predicts guess " << j.front().word
                      << "\n";
            std::cout << "Reduced table expects worst case "
                      << j.front().branches.front().num_bits << "\n";
          }
*/
        }
        state = std::move(b.mask);
        break;
      }
    }
  }
}
