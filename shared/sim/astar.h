#pragma once

#include <cstdint>
#include <vector>

#include "sim/grid.h"

namespace bh::sim {

struct CostGrid {
  int w = 0;
  int h = 0;
  const std::uint8_t* blocked = nullptr;  // row-major, h*w bytes, nonzero = blocked

  bool isBlocked(int x, int y) const {
    if (x < 0 || y < 0 || x >= w || y >= h) return true;
    return blocked[static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x)] != 0;
  }
};

struct PathResult {
  std::vector<TilePos> tiles;  // steps to walk, goal last, start excluded
  int cost = 0;                // fixed-point (100 = cardinal tile)
  bool found = false;
};

// 8-dir A* with no-corner-cutting. Deterministic path shape (stable tie-break).
// maxExpansions bounds worst-case cost on bad maps; overflow reports not-found.
PathResult findPath(const CostGrid& g, TilePos start, TilePos goal, int maxExpansions = 20000);

}  // namespace bh::sim
