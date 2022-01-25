#include <iostream>
#include <string>

#include "partition_map.h"

using namespace wordle;

int main() {
  PartitionMap pm;
  auto sp = pm.SubPartitions(wordle::State::AllBits());
  while (!sp.empty()) {
    std::cout << "I guess: " << sp.front().word << "\n";
    std::string score;
    std::cout << "Score? ";
    std::cin >> score;
    Result r(score);
    for (const Branch& b : sp.front().branches) {
      if (b.result == r) {
        std::cout << b.mask.count() << " left, exemplar " << b.mask.Exemplar()
                  << "\n";
        sp = pm.SubPartitions(b.mask);
        break;
      }
    }
  }
}