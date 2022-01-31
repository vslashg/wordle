#include <iostream>
#include <string>
#include <string_view>

#include "partition_map.h"
#include "reduced_map.h"
#include "score.h"
#include "absl/time/time.h"
#include "absl/time/clock.h"

using namespace wordle;

int main(int argc, char** argv) {
  auto sp = SubPartitions(wordle::State::AllBits());
  if (argc == 2) {
    Word input(argv[1]);
    // forced first guess
    while (!sp.empty()) {
      if (input == sp.front().word) {
        break;
      }
      sp.erase(sp.begin());
    }
  }
  while (!sp.empty()) {
    std::cout << "I guess: " << sp.front().word << "\n";
    std::string colors;
    std::cout << "Colors? ";
    std::cin >> colors;
    Colors c(colors);
    for (const FullBranch& b : sp.front().branches) {
      if (b.colors == c) {
        std::cout << b.mask.count() << " left, exemplar " << b.mask.Exemplar()
                  << "\n";
        if (b.mask.count() < 200) {
          auto time1 = absl::Now();
          ReducedPartitions<4> rp(b.mask);
          auto time2 = absl::Now();
          std::cout << (time2 - time1) / absl::Milliseconds(1) << "ms\n";
          // int score = ScoreState(b.mask);
          // std::cout << "Score " << score << ", EV "
          //           << (double(score) / b.mask.count()) << ", time "
          //           << (time2 - time1) / absl::Milliseconds(1) << "ms\n";
        }
        sp = SubPartitions(b.mask);
        break;
      }
    }
  }
}
