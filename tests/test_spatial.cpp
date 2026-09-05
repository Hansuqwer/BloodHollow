#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "sim/rng.h"
#include "sim/spatial.h"

using namespace bh;

TEST_CASE("spatial grid: query matches brute force across random churn") {
  sim::Rng rng(1337);
  sim::SpatialGrid<8> grid;
  std::unordered_map<std::uint32_t, std::pair<int, int>> truth;

  auto check = [&](int x0, int y0, int x1, int y1) {
    auto hits = grid.query(x0, y0, x1, y1);
    std::sort(hits.begin(), hits.end());
    // grid returns ids in *cells* intersecting the rect; refine to true rect
    std::vector<std::uint32_t> exact;
    for (const std::uint32_t id : hits) {
      const auto [x, y] = truth.at(id);
      if (x >= x0 && x <= x1 && y >= y0 && y <= y1) exact.push_back(id);
    }
    std::vector<std::uint32_t> want;
    for (const auto& [id, p] : truth) {
      if (p.first >= x0 && p.first <= x1 && p.second >= y0 && p.second <= y1) {
        want.push_back(id);
      }
    }
    std::sort(want.begin(), want.end());
    // no false negatives allowed, no false positives beyond cell granularity
    for (const std::uint32_t id : want) {
      CHECK(std::binary_search(hits.begin(), hits.end(), id));
    }
    std::sort(exact.begin(), exact.end());
    CHECK(exact == want);
  };

  // includes negative coords to exercise floorDiv
  for (std::uint32_t id = 1; id <= 200; ++id) {
    const int x = static_cast<int>(rng.range(-100, 100));
    const int y = static_cast<int>(rng.range(-100, 100));
    grid.insert(id, x, y);
    truth[id] = {x, y};
  }
  for (int round = 0; round < 300; ++round) {
    const std::uint32_t id = 1 + static_cast<std::uint32_t>(static_cast<int>(rng.range(0, 199)));
    const int op = static_cast<int>(rng.range(0, 99));
    if (op < 45) {
      const int x = static_cast<int>(rng.range(-100, 100));
      const int y = static_cast<int>(rng.range(-100, 100));
      grid.move(id, x, y);
      truth[id] = {x, y};
    } else if (op < 60) {
      grid.remove(id);
      truth.erase(id);
    } else if (op < 70) {
      const int x = static_cast<int>(rng.range(-100, 100));
      const int y = static_cast<int>(rng.range(-100, 100));
      grid.insert(id, x, y);  // re-insert of existing id: replace, not dup
      truth[id] = {x, y};
    }
    const int x = static_cast<int>(rng.range(-100, 100));
    const int y = static_cast<int>(rng.range(-100, 100));
    const int r = static_cast<int>(rng.range(1, 30));
    check(x - r, y - r, x + r, y + r);
  }
  CHECK(grid.size() == truth.size());
}

TEST_CASE("spatial grid: AoI Chebyshev <=26 window sees the right neighbors") {
  sim::SpatialGrid<8> grid;
  for (std::uint32_t id = 1; id <= 50; ++id) {
    grid.insert(id, static_cast<int>(id) % 7 - 3, static_cast<int>(id) % 5 - 2);
  }
  grid.insert(1000, 25, 25);   // inside Chebyshev 26 of origin-ish
  grid.insert(1001, 33, 0);    // outside; next cell over (cell-adjacent ids pass query by design)
  grid.insert(1002, -26, -26); // exactly on the boundary
  const auto hits = grid.query(-26, -26, 26, 26);
  auto found = [&](std::uint32_t id) {
    return std::find(hits.begin(), hits.end(), id) != hits.end();
  };
  CHECK(found(1));
  CHECK(found(1000));
  CHECK(found(1002));
  CHECK(!found(1001));
}

TEST_CASE("spatial grid: no duplicates after repeated insert of same id") {
  sim::SpatialGrid<8> grid;
  grid.insert(7, 3, 3);
  grid.insert(7, 3, 3);
  grid.insert(7, 40, 40);
  const auto hits = grid.query(0, 0, 50, 50);
  CHECK(hits.size() == 1);
  CHECK(hits[0] == 7u);
  CHECK(grid.size() == 1u);
}
