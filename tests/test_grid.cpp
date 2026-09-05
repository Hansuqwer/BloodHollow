#include <algorithm>
#include <cstdlib>

#include <doctest/doctest.h>

#include "sim/grid.h"

using namespace bh;

TEST_CASE("grid: octile matches 8-dir distance on open ground") {
  for (int dx = -6; dx <= 6; ++dx) {
    for (int dy = -6; dy <= 6; ++dy) {
      const int mn = std::min(std::abs(dx), std::abs(dy));
      const int mx = std::max(std::abs(dx), std::abs(dy));
      CHECK(sim::octile({0, 0}, {dx, dy}) == 100 * mx + 41 * mn);
      // never admissibility-violating: octile <= diag-cost * max
      CHECK(sim::octile({0, 0}, {dx, dy}) <= sim::kDiagonalCost * mx);
    }
  }
}

TEST_CASE("grid: dirIndex maps deltas to the 8 sprite direction rows") {
  CHECK(sim::dirIndex(1, 0) == 0);    // E
  CHECK(sim::dirIndex(1, 1) == 1);    // SE
  CHECK(sim::dirIndex(0, 1) == 2);    // S
  CHECK(sim::dirIndex(-1, 1) == 3);   // SW
  CHECK(sim::dirIndex(-1, 0) == 4);   // W
  CHECK(sim::dirIndex(-1, -1) == 5);  // NW
  CHECK(sim::dirIndex(0, -1) == 6);   // N
  CHECK(sim::dirIndex(1, -1) == 7);   // NE
  CHECK(sim::dirIndex(0, 0) == 2);    // default: S
}
