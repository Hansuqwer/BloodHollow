// T-091 Gravemother telegraphed slam: wind-up arms on the bolt cadence,
// strikes radius 2 after the 60t fuse, movers dodge, stayers eat rot + curse.
#include <doctest/doctest.h>

#include <cstdint>

#include "content/mobs.h"
#include "world.h"

using namespace bh;

namespace {
sim::Map makeArena() {
  sim::Map m;
  m.w = 40;
  m.h = 40;
  m.tileW = 64;
  m.tileH = 32;
  m.ground.assign(40 * 40, 0);
  m.zone.assign(40 * 40, 0);
  m.blocked.assign(40 * 40, 0);
  return m;
}

server::Entity* spawnP(server::World& w, const char* name, int x, int y) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y});
  return w.find(e.id);
}

bool sawKind(const server::World& w, int kind) {
  for (const auto& ev : w.events())
    if (ev.kind == kind) return true;
  return false;
}
}  // namespace

TEST_CASE("T-091: wind-up arms on the bolt cadence, no damage yet") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const content::MobDef* mother = content::findMob(1009);
  REQUIRE(mother != nullptr);
  REQUIRE(mother->boss == 1);
  server::Entity& m = w.debugSpawnMob(*mother, sim::TilePos{10, 10});
  server::Entity* p = spawnP(w, "marked", 13, 10);  // d=3: bolt lane
  p->hpMax = 400;
  p->hp = 400;
  for (int i = 0; i < 400 && !sawKind(w, 15); ++i) w.tick();  // spawn grace + aggro
  REQUIRE(sawKind(w, 15));  // telegraph wind-up announced
  CHECK(m.slamAt > w.tickCount());  // fuse armed in the future
  CHECK(p->hp == 400);              // nothing lands during wind-up
  CHECK(m.slamX == 13);
  CHECK(m.slamY == 10);  // recorded where the victim stood
}

TEST_CASE("T-091: stayers eat rot + curse, movers dodge") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const content::MobDef* mother = content::findMob(1009);
  REQUIRE(mother != nullptr);
  server::Entity& m = w.debugSpawnMob(*mother, sim::TilePos{10, 10});
  server::Entity* stay = spawnP(w, "stayer", 13, 10);
  stay->hpMax = 400;
  stay->hp = 400;
  // The runner starts outside aggro reach (d=15+) and never engages: the
  // wind-up can only mark the stayer, and the strike must miss the runner.
  server::Entity* run = spawnP(w, "runner", 25, 25);
  run->hpMax = 400;
  run->hp = 400;
  for (int i = 0; i < 400 && !sawKind(w, 15); ++i) w.tick();
  REQUIRE(sawKind(w, 15));
  for (int i = 0; i < 120 && !sawKind(w, 16); ++i) w.tick();
  REQUIRE(sawKind(w, 16));          // strike resolved
  CHECK(stay->hp < 400);            // rot landed
  CHECK(stay->curseUntil > w.tickCount());  // thin blood (T-070 lane)
  CHECK(run->hp == 400);            // dodged clean
  CHECK(m.slamAt < 0);              // wind-up disarmed
}

TEST_CASE("T-091: losing the mark cancels the wind-up") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const content::MobDef* mother = content::findMob(1009);
  REQUIRE(mother != nullptr);
  server::Entity& m = w.debugSpawnMob(*mother, sim::TilePos{10, 10});
  server::Entity* p = spawnP(w, "gone", 13, 10);
  p->hpMax = 400;
  p->hp = 400;
  for (int i = 0; i < 400 && !sawKind(w, 15); ++i) w.tick();
  REQUIRE(sawKind(w, 15));
  w.debugKillPlayer(*p);  // mark dies mid-fuse (leash-loss equivalent)
  for (int i = 0; i < 5; ++i) w.tick();
  CHECK(m.slamAt < 0);  // cancelled, never strikes the corpse tile twice
  // Stay inside the 100t spawn grace: no re-aggro, no second wind-up.
  // (events clear per tick, so watch every tick — a strike on any tick fails.)
  int quiet = 0;
  for (int i = 0; i < 55 && !sawKind(w, 16); ++i) {
    w.tick();
    ++quiet;
  }
  CHECK(quiet == 55);
  CHECK(m.slamAt < 0);
  CHECK_FALSE(sawKind(w, 16));
}
