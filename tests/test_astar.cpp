#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <queue>
#include <vector>

#include <doctest/doctest.h>

#include "sim/astar.h"
#include "sim/rng.h"

using namespace bh;

namespace {

// Reference shortest-path (Dijkstra, same cost model + corner rule).
int dijkstra(const sim::CostGrid& g, sim::TilePos s, sim::TilePos t) {
  if (g.isBlocked(s.x, s.y) || g.isBlocked(t.x, t.y)) return -1;
  const int w = g.w;
  const int n = w * g.h;
  const int kInf = std::numeric_limits<int>::max();
  std::vector<int> dist(static_cast<size_t>(n), kInf);
  using Node = std::pair<int, int>;
  std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;
  dist[static_cast<size_t>(s.y * w + s.x)] = 0;
  pq.push({0, s.y * w + s.x});
  while (!pq.empty()) {
    const Node cur = pq.top();
    pq.pop();
    const int d = cur.first;
    const int i = cur.second;
    if (d > dist[static_cast<size_t>(i)]) continue;
    const int x = i % w;
    const int y = i / w;
    if (x == t.x && y == t.y) return d;
    for (int dir = 0; dir < 8; ++dir) {
      const int nx = x + sim::kDx[dir];
      const int ny = y + sim::kDy[dir];
      if (g.isBlocked(nx, ny)) continue;
      const bool diag = sim::kDx[dir] != 0 && sim::kDy[dir] != 0;
      if (diag && (g.isBlocked(x, ny) || g.isBlocked(nx, y))) continue;
      const int nd = d + (diag ? sim::kDiagonalCost : sim::kCardinalCost);
      int& slot = dist[static_cast<size_t>(ny * w + nx)];
      if (nd < slot) {
        slot = nd;
        pq.push({nd, ny * w + nx});
      }
    }
  }
  return -1;
}

}  // namespace

TEST_CASE("astar: corner cutting is forbidden") {
  // (1,0) blocked: (0,0)->(2,1) cannot cut through (1,1) diagonally.
  std::vector<std::uint8_t> b = {0, 1, 0, 0, 0, 0, 0, 0, 0};
  const sim::CostGrid g{3, 3, b.data()};
  const sim::PathResult r = sim::findPath(g, {0, 0}, {2, 1});
  REQUIRE(r.found);
  CHECK(r.cost == 300);  // (0,1) -> (1,1) -> (2,1)
  CHECK(r.tiles.size() == 3);
}

TEST_CASE("astar: unreachable goal reports not-found") {
  // start (0,0) fully walled in by neighbours incl. corner-cut blocks
  std::vector<std::uint8_t> b = {0, 1, 1, 1, 1, 0, 1, 0, 0};
  const sim::CostGrid g{3, 3, b.data()};
  CHECK_FALSE(sim::findPath(g, {0, 0}, {2, 2}).found);
}

TEST_CASE("astar: start == goal yields an empty found path") {
  std::vector<std::uint8_t> b(9, 0);
  const sim::CostGrid g{3, 3, b.data()};
  const sim::PathResult r = sim::findPath(g, {1, 1}, {1, 1});
  CHECK(r.found);
  CHECK(r.cost == 0);
  CHECK(r.tiles.empty());
}

TEST_CASE("astar: 1000 random queries match Dijkstra reference costs") {
  const int w = 64;
  const int h = 64;
  std::vector<std::uint8_t> store(static_cast<size_t>(w * h), 0);
  {
    sim::Rng rng(0xB100D);
    for (auto& v : store) v = rng.chance(0.20) ? 1 : 0;
  }
  const sim::CostGrid g{w, h, store.data()};

  sim::Rng qrng(0xBEEF);
  int checked = 0;
  int unreachable = 0;
  for (int i = 0; i < 1000; ++i) {
    const sim::TilePos s{static_cast<int>(qrng.range(0, w - 1)),
                         static_cast<int>(qrng.range(0, h - 1))};
    const sim::TilePos t{static_cast<int>(qrng.range(0, w - 1)),
                         static_cast<int>(qrng.range(0, h - 1))};
    if (s == t) continue;
    const int expect = dijkstra(g, s, t);
    const sim::PathResult r = sim::findPath(g, s, t);
    if (expect < 0) {
      CHECK_FALSE(r.found);
      ++unreachable;
      continue;
    }
    REQUIRE(r.found);
    CHECK(r.cost == expect);
    // Path legality: adjacency, walkability, corner rule, cost recompute.
    sim::TilePos cur = s;
    int recompute = 0;
    for (const sim::TilePos& step : r.tiles) {
      const int dx = step.x - cur.x;
      const int dy = step.y - cur.y;
      REQUIRE(std::abs(dx) <= 1);
      REQUIRE(std::abs(dy) <= 1);
      const bool isStep = dx != 0 || dy != 0;
      REQUIRE(isStep);
      CHECK_FALSE(g.isBlocked(step.x, step.y));
      const bool diag = dx != 0 && dy != 0;
      const bool cornerCut = diag && (g.isBlocked(cur.x, step.y) || g.isBlocked(step.x, cur.y));
      CHECK_FALSE(cornerCut);
      recompute += diag ? sim::kDiagonalCost : sim::kCardinalCost;
      cur = step;
    }
    CHECK(cur == t);
    CHECK(recompute == r.cost);
    ++checked;
  }
  // At 20% density roughly a third of random pairs are disconnected; these
  // bounds just prove the property test exercised many real queries.
  CHECK(checked > 500);
  CHECK(unreachable < 450);
}
