#include <iostream>
#include <string>
#include <string_view>

#include "partition_map.h"
#include "score.h"

using namespace wordle;

int main(int argc, char** argv) {
  const FullPartitionMap& pm = FullPartitionMap::Singleton();
  auto sp = pm.SubPartitions(wordle::State::AllBits());
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
        if (b.mask.count() < 100) {
          int score = ScoreState(b.mask);
          std::cout << "Score " << score << ", EV "
                    << (double(score) / b.mask.count()) << "\n";
        }
        sp = pm.SubPartitions(b.mask);
        break;
      }
    }
  }
}
