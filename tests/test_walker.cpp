#include <deque>
#include <vector>

#include <doctest/doctest.h>

#include "sim/astar.h"
#include "sim/walker.h"

using namespace bh;

namespace {
sim::CostGrid openGrid(std::vector<std::uint8_t>* store, int w, int h) {
  store->assign(static_cast<size_t>(w * h), 0);
  return sim::CostGrid{w, h, store->data()};
}
}  // namespace

TEST_CASE("walker: cardinal step takes exactly 4 ticks (era pace)") {
  std::vector<std::uint8_t> store;
  const auto g = openGrid(&store, 8, 8);
  sim::Walker w;
  w.place({1, 1});
  REQUIRE(w.beginStep(g, {2, 1}));
  int arrivalTick = -1;
  for (int t = 1; t <= 5; ++t) {
    if (w.step()) arrivalTick = t;
  }
  CHECK(arrivalTick == 4);
  CHECK(w.tile() == sim::TilePos{2, 1});
  CHECK_FALSE(w.moving);
}

TEST_CASE("walker: refuses blocked tiles, leaps >1, and corner cuts") {
  // (1,0) and (0,1) blocked around origin (0,0); (1,1) free but unreachable.
  std::vector<std::uint8_t> b = {0, 1, 0, 1, 0, 0, 0, 0, 0};
  const sim::CostGrid g{3, 3, b.data()};
  sim::Walker w;
  w.place({0, 0});
  CHECK_FALSE(w.beginStep(g, {1, 0}));  // wall
  CHECK_FALSE(w.beginStep(g, {0, 1}));  // wall
  CHECK_FALSE(w.beginStep(g, {1, 1}));  // corner cut
  CHECK_FALSE(w.beginStep(g, {2, 2}));  // too far
  CHECK(w.beginStep(g, {0, 0}) == false);  // no-op step
}

TEST_CASE("walker: deterministic double-run hashes (the replay oracle)") {
  std::vector<std::uint8_t> store;
  const auto g = openGrid(&store, 30, 30);
  const auto route = sim::findPath(g, {2, 3}, {25, 27});
  REQUIRE(route.found);

  auto run = [&]() {
    sim::Walker w;
    w.place({2, 3});
    std::deque<sim::TilePos> path(route.tiles.begin(), route.tiles.end());
    std::vector<std::uint64_t> hashes;
    for (int i = 0; i < 400; ++i) {
      if (!w.moving && !path.empty()) {
        const sim::TilePos t = path.front();
        if (w.beginStep(g, t)) path.pop_front();
      }
      w.step();
      hashes.push_back(sim::walkerStateHash(w, path));
    }
    return std::pair{hashes, w.tile()};
  };
  const auto a = run();
  const auto b = run();
  CHECK(a.first == b.first);  // bit-identical trajectories
  CHECK(a.second == sim::TilePos{25, 27});
}
