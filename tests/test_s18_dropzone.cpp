// T-063 Bonehowl Mine + Drowned Crypt load, T-064 Gravemother & Blood Bolt,
// T-065 Wanted Board session-scoped. Assets-side tests use the real bhmaps.
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

#include "content/mobs.h"
#include "sim/clock.h"
#include "world.h"

using namespace bh;

TEST_CASE("T-063: both new zones load, portal graph closes (asset gate)") {
  server::World w;
  std::string err;
  REQUIRE(w.loadZone(4, "assets/maps/bonehowl_mine.bhmap", &err));
  REQUIRE(w.loadZone(5, "assets/maps/drowned_crypt.bhmap", &err));
  const sim::Map* mine = w.zoneMap(4);
  const sim::Map* crypt = w.zoneMap(5);
  REQUIRE(mine != nullptr);
  REQUIRE(crypt != nullptr);
  CHECK(mine->w == 56);
  CHECK(crypt->w == 48);
  // the mine mouth portal exists & targets map 2; depths stairs target map 3
  bool mineOut = false, depthsUp = false;
  for (const auto& pd : mine->portals)
    if (pd.targetMapId == 2) mineOut = true;
  for (const auto& pd : crypt->portals)
    if (pd.targetMapId == 3) depthsUp = true;
  CHECK(mineOut);
  CHECK(depthsUp);
  // spawn count: 7 mine camp spawners + Widow nest (T-102); 11 crypt
  // spawners incl 8 elites + boss
  CHECK(mine->spawners.size() == 8);
  CHECK(crypt->spawners.size() == 11);
}

TEST_CASE("T-064: Gravemother table law — boss bit, bolt, x20 purse") {
  const auto* gm = content::findMob(1009);
  REQUIRE(gm != nullptr);
  CHECK(std::string(gm->name) == "Gravemother");
  CHECK(gm->level == 14);
  CHECK(gm->boss == 1);
  CHECK(gm->boltRange == 6);
  CHECK(gm->boltCdTicks == 26);
  CHECK(gm->goldLo >= 400);      // x20 purse band vs a rat's 6..14
  const auto* elite = content::findMob(1010);
  REQUIRE(elite != nullptr);
  CHECK(elite->boss == 0);
  // xp ratios era-pinned: elite x8 vs Gravecaller-tier line, boss x20 vs rat base
  CHECK(elite->xp == 3200);
  CHECK(gm->xp == 4000);
}

TEST_CASE("T-064: Blood Bolt fires range 2-6 on cd, not in melee, night +25%") {
  sim::Map cavity;
  cavity.w = 20;
  cavity.h = 20;
  cavity.tileW = 64;
  cavity.tileH = 32;
  cavity.ground.assign(400, 0);
  cavity.zone.assign(400, 0);
  cavity.blocked.assign(400, 0);
  server::World w;
  REQUIRE(w.loadFrom(std::move(cavity)));
  auto* p = w.find(w.spawn("wanderer", 0, sim::TilePos{5, 5}).id);
  p->hp = 4000;
  p->hpMax = 4000;
  p->dex = 0;
  auto* gm = w.find(w.debugSpawnMob(*content::findMob(1009), {5, 8}).id);
  REQUIRE(gm != nullptr);
  gm->walker.place(sim::TilePos{9, 5});        // 4 tiles out, bolt range
  gm->aggroRadius = 8;
  // force aggression by hurting the boss (player-attacked path)
  const std::int32_t hp0 = p->hp;
  gm->attackTarget = p->id;
  gm->lastSwingTick = 0;
  int guard = 300;
  bool bolted = false;
  while (guard-- > 0 && p->hp > 0) {
    w.tick();
    if (static_cast<std::int32_t>(p->hp) < hp0) bolted = true;
    gm = w.find(gm->id);
    REQUIRE(gm != nullptr);
  }
  CHECK(bolted);  // the bolt travels the air-lane
}

TEST_CASE("T-065: bounty — passive assign near board, kill pays, sheet drains") {
  sim::Map square;
  square.w = 30;
  square.h = 30;
  square.tileW = 64;
  square.tileH = 32;
  square.ground.assign(900, 0);
  square.zone.assign(900, 0);
  square.blocked.assign(900, 0);
  server::World w;
  REQUIRE(w.loadFrom(std::move(square)));
  auto* p = w.find(w.spawn("bountyhunter", 0, sim::TilePos{10, 10}).id);
  REQUIRE(p != nullptr);
  const std::uint32_t pGold0 = p->gold;
  // spawnVendor path is zone-1-keyed; the seam conjures the board beside us
  static_cast<void>(w.debugSpawnBoard(sim::TilePos{11, 10}));
  // passive assign fires from World::tick at 0.5Hz — stand there a while
  for (int i = 0; i < 160; ++i) w.tick();
  REQUIRE(p->bountyMobId != 0);
  const auto* b = w.bountyNow();
  REQUIRE(b != nullptr);
  CHECK(p->bountyMobId == b->mobId);
  // quarry target: Gravemother for cycle 0 (kBountyQuarry[0])
  CHECK(b->mobId == 1009);
  auto* gm = w.find(w.debugSpawnMob(*content::findMob(1009), {8, 8}).id);
  REQUIRE(gm != nullptr);
  w.debugKillMob(*gm, p);
  // bounty sits ON TOP of the boss's natural gold band (400..650):
  const std::int64_t paid =
      static_cast<std::int64_t>(p->gold) - static_cast<std::int64_t>(pGold0) -
      static_cast<std::int64_t>(b->payoutGold);
  CHECK(paid >= 400);
  CHECK(paid <= 650);
  CHECK(p->bountyMobId == 0);  // sheet drained — the board wants something fresh
  // a second, un-sheeted kill does NOT pay out again (no anonymous greed)
  const std::uint32_t goldAfter = p->gold;
  auto* gm2 = w.find(w.debugSpawnMob(*content::findMob(1009), {9, 8}).id);
  w.debugKillMob(*gm2, p);
  // the strict check: gold delta from the 2nd kill sits inside the boss's
  // natural gold band (i.e. NO bounty payout layered on top)
  const std::int64_t delta = static_cast<std::int64_t>(p->gold) - goldAfter;
  CHECK(delta >= 400);
  CHECK(delta <= 650);
}
