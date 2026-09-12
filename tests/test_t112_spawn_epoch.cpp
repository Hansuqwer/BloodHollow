// T-112: epoch-19 spawn-semantics pins (T-111 F4 follow-up).
//
// T-111 shipped the initialMobSpawns bounded scatter claiming replay
// neutrality; master's logs/t107.bwj leg disproved that (81/81 hash
// mismatches from the first checkpoint, entities 196 vs 169): the old
// `--n`-on-blocked-roll wrap aborted whole spawners, so filling them under
// T-111 shifts the boot RNG stream and every world hash after it. That is a
// spawn-semantics change -> kJournalEpoch 19 + fresh gate leg
// logs/t112.bwj (this card). These tests pin the NEW law so it cannot
// silently drift again:
//   F4a — a fully-blocked spawner rect terminates and places nothing
//         (maxTries cap; the pre-T-111 wrap broke out here too, but via UB)
//   F4b — a half-blocked rect still fills maxAlive, all on walkable tiles
//         (pre-T-111: a blocked first roll aborted the spawner -> 0 mobs)
//   F4c — a rect with a single free tile still fills maxAlive
//         (bounded re-roll; stacking on the one walkable tile is legal)
#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <utility>

#include "sim/bhmap.h"
#include "world.h"

using namespace bh;

namespace {
sim::Map makeMap(std::int32_t w, std::int32_t h) {
  sim::Map m;
  m.w = w;
  m.h = h;
  m.tileW = 64;
  m.tileH = 32;
  m.ground.assign(static_cast<size_t>(w) * h, 0);
  m.zone.assign(static_cast<size_t>(w) * h, 0);
  m.blocked.assign(static_cast<size_t>(w) * h, 0);
  return m;
}

void block(sim::Map& m, int x0, int y0, int x1, int y1) {
  for (int y = y0; y <= y1; ++y)
    for (int x = x0; x <= x1; ++x)
      m.blocked[static_cast<size_t>(y) * m.w + x] = 1;
}

std::uint32_t zoneEntities(server::World& w, std::uint16_t zone) {
  std::uint32_t n = 0;
  for (const auto& e : w.entities())
    if (e.zoneId == zone) ++n;
  return n;
}
}  // namespace

TEST_CASE("T-112 F4a: fully-blocked spawner rect terminates, places nothing") {
  sim::Map m = makeMap(20, 20);
  sim::SpawnDef sd;
  sd.x = 5;
  sd.y = 5;
  sd.w = 4;
  sd.h = 4;
  sd.mobId = 1001;  // Marsh Rat (content table ships 1001..1014)
  sd.maxAlive = 5;
  m.spawners.push_back(sd);
  block(m, 5, 5, 8, 8);  // the whole rect is solid rock
  server::World w;
  REQUIRE(w.loadZoneFrom(7, std::move(m)));  // returns: no wrap, no spin
  CHECK(zoneEntities(w, 7) == 0);
}

TEST_CASE("T-112 F4b: half-blocked rect fills maxAlive on walkable tiles") {
  sim::Map m = makeMap(20, 20);
  sim::SpawnDef sd;
  sd.x = 5;
  sd.y = 5;
  sd.w = 4;
  sd.h = 4;
  sd.mobId = 1001;
  sd.maxAlive = 6;
  m.spawners.push_back(sd);
  block(m, 5, 5, 6, 8);  // left half blocked, right half walkable
  server::World w;
  REQUIRE(w.loadZoneFrom(7, std::move(m)));
  CHECK(zoneEntities(w, 7) == 6);  // fills despite the blocked rolls
  for (int y = 5; y <= 8; ++y)     // nothing ever placed on solid rock
    for (int x = 5; x <= 6; ++x) CHECK(w.queryAoi(7, x, y, 0).empty());
}

TEST_CASE("T-112 F4c: single-free-tile rect still fills (bounded re-roll)") {
  sim::Map m = makeMap(20, 20);
  sim::SpawnDef sd;
  sd.x = 5;
  sd.y = 5;
  sd.w = 4;
  sd.h = 4;
  sd.mobId = 1001;
  sd.maxAlive = 3;
  m.spawners.push_back(sd);
  block(m, 5, 5, 8, 8);
  m.blocked[6 * m.w + 6] = 0;  // exactly one walkable tile in the rect
  server::World w;
  REQUIRE(w.loadZoneFrom(7, std::move(m)));
  CHECK(zoneEntities(w, 7) == 3);
  CHECK(w.queryAoi(7, 6, 6, 0).size() == 3);  // all three on the free tile
}
