#include "sim/astar.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <utility>

namespace bh::sim {

PathResult findPath(const CostGrid& g, TilePos start, TilePos goal, int maxExpansions) {
  PathResult out;
  if (g.blocked == nullptr || g.w <= 0 || g.h <= 0) return out;
  if (g.isBlocked(start.x, start.y) || g.isBlocked(goal.x, goal.y)) return out;
  if (start == goal) {
    out.found = true;
    return out;
  }

  const int w = g.w;
  const int h = g.h;
  const int n = w * h;
  auto idx = [w](int x, int y) { return y * w + x; };

  constexpr int kInf = std::numeric_limits<int>::max();
  std::vector<int> gCost(static_cast<size_t>(n), kInf);
  std::vector<int> parent(static_cast<size_t>(n), -1);
  std::vector<std::uint8_t> closed(static_cast<size_t>(n), 0);

  // (fCost, cellIndex); smallest f first, ties by index for determinism.
  using Node = std::pair<int, int>;
  std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;

  gCost[static_cast<size_t>(idx(start.x, start.y))] = 0;
  open.push({octile(start, goal), idx(start.x, start.y)});

  int expansions = 0;
  while (!open.empty()) {
    const Node cur = open.top();
    open.pop();
    const int ci = cur.second;
    if (closed[static_cast<size_t>(ci)] != 0) continue;
    closed[static_cast<size_t>(ci)] = 1;
    const int cx = ci % w;
    const int cy = ci / w;

    if (cx == goal.x && cy == goal.y) {
      out.cost = gCost[static_cast<size_t>(ci)];
      out.found = true;
      std::vector<TilePos> rev;
      for (int p = ci; p != idx(start.x, start.y); p = parent[static_cast<size_t>(p)]) {
        rev.push_back(TilePos{p % w, p / w});
      }
      std::reverse(rev.begin(), rev.end());
      out.tiles = std::move(rev);
      return out;
    }
    if (++expansions > maxExpansions) return out;

    for (int d = 0; d < 8; ++d) {
      const int nx = cx + kDx[d];
      const int ny = cy + kDy[d];
      if (g.isBlocked(nx, ny)) continue;
      const bool diag = (kDx[d] != 0 && kDy[d] != 0);
      if (diag && (g.isBlocked(cx, ny) || g.isBlocked(nx, cy))) continue;  // no corner cuts
      const int ni = idx(nx, ny);
      if (closed[static_cast<size_t>(ni)] != 0) continue;
      const int ng = gCost[static_cast<size_t>(ci)] + (diag ? kDiagonalCost : kCardinalCost);
      if (ng < gCost[static_cast<size_t>(ni)]) {
        gCost[static_cast<size_t>(ni)] = ng;
        parent[static_cast<size_t>(ni)] = ci;
        open.push({ng + octile(TilePos{nx, ny}, goal), ni});
      }
    }
  }
  return out;
}

}  // namespace bh::sim
