#include <iostream>
#include <string>
#include <string_view>

#include "partition_map.h"

using namespace wordle;

int main(int argc, char** argv) {
  PartitionMap pm;
  auto sp = pm.SubPartitions(wordle::State::AllBits());
  if (argc == 2) {
    // forced first guess
    while (!sp.empty()) {
      if (std::string_view(argv[1]) == sp.front().word) {
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
    for (const Branch& b : sp.front().branches) {
      if (b.colors == c) {
        std::cout << b.mask.count() << " left, exemplar " << b.mask.Exemplar()
                  << "\n";
        sp = pm.SubPartitions(b.mask);
        break;
      }
    }
  }
}
