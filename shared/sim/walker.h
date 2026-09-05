#pragma once

#include <cstdint>
#include <cstdlib>
#include <deque>

#include "sim/astar.h"
#include "sim/grid.h"

namespace bh::sim {

// Fixed-point (Q10) one-tile-step movement primitive shared by the client
// hero now and every server entity from Phase 1 on. Integer-only state means
// bit-stable world hashes across platforms (ADR-005) — the replay oracle.
class Walker {
 public:
  static constexpr int kUnitsPerTile = 1024;   // Q10 tile-space unit
  static constexpr int kUnitsPerTick = 256;    // 4 ticks per tile = era pace

  int x = 0;  // Q10 position (tile index * 1024)
  int y = 0;
  TilePos target{};
  bool moving = false;
  int dir = 2;  // facing, sprite row order via grid.h

  void place(TilePos t) {
    x = t.x * kUnitsPerTile;
    y = t.y * kUnitsPerTile;
    moving = false;
  }
  TilePos tile() const { return TilePos{(x + 512) >> 10, (y + 512) >> 10}; }
  float fx() const { return static_cast<float>(x) / static_cast<float>(kUnitsPerTile); }
  float fy() const { return static_cast<float>(y) / static_cast<float>(kUnitsPerTile); }

  // Begin a one-tile step toward t if legal (adjacent, walkable, no corner cut).
  bool beginStep(const CostGrid& g, TilePos t) {
    if (moving) return false;
    if (g.isBlocked(t.x, t.y)) return false;
    const TilePos cur = tile();
    const int dx = t.x - cur.x;
    const int dy = t.y - cur.y;
    if (dx == 0 && dy == 0) return false;
    if (std::abs(dx) > 1 || std::abs(dy) > 1) return false;
    if (dx != 0 && dy != 0 && (g.isBlocked(cur.x + dx, cur.y) || g.isBlocked(cur.x, cur.y + dy))) {
      return false;
    }
    target = t;
    dir = dirIndex(dx, dy);
    moving = true;
    return true;
  }

  // Advance one sim tick. Returns true exactly on the arrival tick.
  bool step() {
    if (!moving) return false;
    const int tx = target.x * kUnitsPerTile;
    const int ty = target.y * kUnitsPerTile;
    const int dx = tx - x;
    const int dy = ty - y;
    const int ax = std::abs(dx);
    const int ay = std::abs(dy);
    if (ax <= kUnitsPerTick && ay <= kUnitsPerTick) {
      x = tx;
      y = ty;
      moving = false;
      return true;
    }
    x += (dx > 0 ? 1 : -1) * (ax < kUnitsPerTick ? ax : kUnitsPerTick);
    y += (dy > 0 ? 1 : -1) * (ay < kUnitsPerTick ? ay : kUnitsPerTick);
    return false;
  }
};

// Bit-stable state hash (FNV-1a over all sim-visible fields + queued path).
std::uint64_t walkerStateHash(const Walker& w, const std::deque<TilePos>& path);

}  // namespace bh::sim
