#pragma once

#include <cstdint>
#include <cstdlib>

namespace bh::sim {

// 8-direction movement table. Order doubles as sprite direction rows:
// 0=E, 1=SE, 2=S, 3=SW, 4=W, 5=NW, 6=N, 7=NE (clockwise from East,
// y grows "south" = down-screen in our iso projection).
extern const int kDx[8];
extern const int kDy[8];

// Fixed-point movement costs (100 = one cardinal tile).
inline constexpr int kCardinalCost = 100;
inline constexpr int kDiagonalCost = 141;  // ~sqrt(2)*100

struct TilePos {
  std::int32_t x = 0;
  std::int32_t y = 0;
  bool operator==(const TilePos&) const = default;
};

// Octile heuristic; admissible for our cost model and exact on open ground.
inline int octile(TilePos a, TilePos b) {
  const int dx = std::abs(a.x - b.x);
  const int dy = std::abs(a.y - b.y);
  const int mn = dx < dy ? dx : dy;
  const int mx = dx < dy ? dy : dx;
  return kCardinalCost * mx + (kDiagonalCost - kCardinalCost) * mn;
}

// Direction index (0..7) from a delta, for animation facing. Ties resolve
// toward the table order above. (0,0) yields South as a sane default.
inline int dirIndex(int dx, int dy) {
  const int sx = (dx > 0) - (dx < 0);
  const int sy = (dy > 0) - (dy < 0);
  for (int i = 0; i < 8; ++i) {
    if (kDx[i] == sx && kDy[i] == sy) return i;
  }
  return 2;
}

}  // namespace bh::sim
