// T-131 siege scheduler + registration + holder slot (Phase S, 1/4).
// Pins: window edges/periodicity/anchor, registration matrix, holder
// default, battle gating, journaled paths.
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

#include "command.h"
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
}  // namespace

TEST_CASE("T-131: window edges are Saturday 20:00-21:30") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  CHECK(w.siegeWindowStart() == 1872000);
  w.debugSetTick(1871999);
  CHECK_FALSE(w.inSiegeWindow());
  w.debugSetTick(1872000);
  CHECK(w.inSiegeWindow());
  REQUIRE(w.siegeWindowEnd() == 1980000);
  w.debugSetTick(1979999);
  CHECK(w.inSiegeWindow());
  w.debugSetTick(1980000);
  CHECK_FALSE(w.inSiegeWindow());
}

TEST_CASE("T-131: the window repeats weekly from the 08:00 anchor") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  w.debugSetTick(1872000 + 2016000);  // one week later
  CHECK(w.inSiegeWindow());
  CHECK(w.siegeWindowStart() == 1872000 + 2016000);
  w.debugSetTick(500);  // Monday-morning nowhere near Saturday
  CHECK_FALSE(w.inSiegeWindow());
}

TEST_CASE("T-131: registration matrix") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "captain", 5, 5);
  CHECK(w.siegeRegister(a->id));
  CHECK(w.siegeAttackers().size() == 1u);
  CHECK_FALSE(w.siegeRegister(a->id));  // dedup quiet
  CHECK(w.siegeAttackers().size() == 1u);
  CHECK_FALSE(w.siegeRegister(999999u));  // unknown id
  a->dead = true;
  server::Entity* b = spawnP(w, "second", 6, 5);
  (void)b;
  CHECK_FALSE(w.siegeRegister(a->id));  // the dead raise no bands
  // camp fills at 8
  for (int i = 0; i < 7; ++i) {
    server::Entity* p = spawnP(w, ("band" + std::to_string(i)).c_str(), 10 + i, 10);
    REQUIRE(w.siegeRegister(p->id));
  }
  CHECK(w.siegeAttackers().size() == 8u);
  server::Entity* extra = spawnP(w, "ninth", 30, 30);
  CHECK_FALSE(w.siegeRegister(extra->id));  // camp is full
}

TEST_CASE("T-131: holder defaults unclaimed, battle gates hold") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  CHECK(w.siegeHolder() == 0u);
  CHECK_FALSE(w.siegeBattleActive());
  server::Entity* a = spawnP(w, "captain", 5, 5);
  w.debugSetTick(500);  // out of window
  REQUIRE(w.siegeRegister(a->id));
  CHECK_FALSE(w.siegeStart(*a));  // out of window: quiet
  CHECK_FALSE(w.siegeBattleActive());
  w.debugSetTick(1872000);  // Saturday 20:00
  server::World w2;
  REQUIRE(w2.loadFrom(makeArena()));
  server::Entity* lone = spawnP(w2, "lonely", 5, 5);
  w2.debugSetTick(1872000);
  CHECK_FALSE(w2.siegeStart(*lone));  // no bands: quiet
  REQUIRE(w.siegeStart(*a));
  CHECK(w.siegeBattleActive());
  CHECK_FALSE(w.siegeStart(*a));  // one battle at a time: quiet
  a->dead = true;
  CHECK_FALSE(w.siegeStart(*a));
}

TEST_CASE("T-131: battle ends at window close") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "captain", 5, 5);
  REQUIRE(w.siegeRegister(a->id));
  w.debugSetTick(1872000);
  REQUIRE(w.siegeStart(*a));
  w.debugSetTick(1979998);
  w.tick();  // now 1979999: inside, battle holds
  CHECK(w.siegeBattleActive());
  w.debugSetTick(1980000);
  w.tick();  // now 1980001: past the close
  CHECK_FALSE(w.siegeBattleActive());
}

TEST_CASE("T-131: journaled registration and start") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "roundtrip", 5, 5);
  server::Command c;
  c.kind = server::Command::kSiegeReg;
  server::applyWorldCommand(w, *a, c);
  CHECK(w.siegeAttackers().size() == 1u);
  w.debugSetTick(1872000);
  server::Command s;
  s.kind = server::Command::kSiegeStart;
  server::applyWorldCommand(w, *a, s);
  CHECK(w.siegeBattleActive());
}

TEST_CASE("T-136: rehearsal posture opens the window unconditionally") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  CHECK_FALSE(w.rehearsalMode());
  w.debugSetTick(500);  // Monday nowhere near Saturday
  CHECK_FALSE(w.inSiegeWindow());
  w.setRehearsal(true);
  CHECK(w.rehearsalMode());
  CHECK(w.inSiegeWindow());
  w.setRehearsal(false);
  CHECK_FALSE(w.rehearsalMode());
  CHECK_FALSE(w.inSiegeWindow());
}

TEST_CASE("T-137: registration enlists the captain's living party as one band") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "leader", 5, 5);
  server::Entity* b = spawnP(w, "mate", 6, 5);
  server::Entity* c = spawnP(w, "third", 5, 6);
  REQUIRE(w.partyInvite(*a, *b));
  REQUIRE(w.partyAccept(*b));
  REQUIRE(w.partyInvite(*a, *c));
  REQUIRE(w.partyAccept(*c));
  REQUIRE(w.siegeRegister(a->id));  // one call, three enlisted, one band
  CHECK(w.siegeAttackers().size() == 3u);
  CHECK_FALSE(w.siegeRegister(b->id));  // already in camp
  // dead members ride no band
  w.debugKillPlayer(*c);
  server::Entity* d = spawnP(w, "lonely", 8, 8);
  server::Entity* e = spawnP(w, "fallen", 9, 8);
  REQUIRE(w.partyInvite(*d, *e));
  REQUIRE(w.partyAccept(*e));
  w.debugKillPlayer(*e);
  REQUIRE(w.siegeRegister(d->id));
  CHECK(w.siegeAttackers().size() == 4u);  // d only; e was dead
  // the camp fills at 8 BANDS: 6 more solo registrations, 9th refused
  for (int i = 0; i < 6; ++i) {
    server::Entity* p = spawnP(w, ("solo" + std::to_string(i)).c_str(), 20 + i, 20);
    REQUIRE(w.siegeRegister(p->id));
  }
  server::Entity* extra = spawnP(w, "ninth", 30, 30);
  CHECK_FALSE(w.siegeRegister(extra->id));  // 9th band: camp is full
}
