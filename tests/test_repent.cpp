// T-075 karma repentance: the chapel's second lane (grace, not cure).
// Pins derive from shipped numbers, not invention: +20 karma is one
// whitening-hour at the pinned rate; the 72000-tick cooldown is one logged
// hour, the whitening denominator's own unit. Curse untouched; wanted
// refused (gate law and chapel grace stay separate).
#include <doctest/doctest.h>

#include <cstdint>

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

server::Entity* confessorAt(server::World& w, server::Entity* p) {
  (void)w.debugSpawnConfessor(
      sim::TilePos{p->walker.tile().x + 1, p->walker.tile().y});
  return p;
}
}  // namespace

TEST_CASE("T-075: repentance grants one whitening-hour (+20 karma)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "sinner", 10, 10);
  confessorAt(w, p);
  p->karma = -100;
  REQUIRE(w.repent(*p));
  CHECK(p->karma == -80);
  CHECK(w.find(p->id)->repentUntil == w.tickCount() + 72000);
}

TEST_CASE("T-075: clamp at the rails, band events fire") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "edger", 10, 10);
  confessorAt(w, p);
  p->karma = 995;
  REQUIRE(w.repent(*p));
  CHECK(p->karma == 1000);  // bumpKarma clamps ±1000
  p->karma = -1000;
  p->repentUntil = -1;
  REQUIRE(w.repent(*p));
  CHECK(p->karma == -980);
}

TEST_CASE("T-075: grace cools hourly — exact edge") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "cooled", 10, 10);
  confessorAt(w, p);
  p->karma = 0;
  const sim::Tick t0 = w.tickCount();
  REQUIRE(w.repent(*p));
  CHECK(p->karma == 20);
  w.debugSetTick(t0 + 71999);
  CHECK_FALSE(w.repent(*w.find(p->id)));  // still cooling
  w.debugSetTick(t0 + 72000);
  CHECK(w.repent(*w.find(p->id)));  // edge: grace again
  CHECK(w.find(p->id)->karma == 40);
}

TEST_CASE("T-075: far from the chapel the words find no purchase") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  (void)w.debugSpawnConfessor(sim::TilePos{10, 10});
  server::Entity* p = spawnP(w, "far", 30, 30);
  p->karma = -50;
  CHECK_FALSE(w.repent(*p));
  CHECK(p->karma == -50);
}

TEST_CASE("T-075: the wanted are refused (lanes stay separate)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "bound", 10, 10);
  confessorAt(w, p);
  p->karma = -50;
  p->wantedUntil = w.tickCount() + 4800;
  CHECK_FALSE(w.repent(*p));
  CHECK(p->karma == -50);
}

TEST_CASE("T-075: grace never touches the curse") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "cursed", 10, 10);
  confessorAt(w, p);
  p->karma = -50;
  p->curseUntil = w.tickCount() + 600;
  REQUIRE(w.repent(*p));
  CHECK(p->karma == -30);
  CHECK(p->curseUntil > w.tickCount());  // still thin-blooded
}

TEST_CASE("T-075: /repent through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "roundtrip", 10, 10);
  confessorAt(w, p);
  p->karma = 0;
  server::Command c;
  c.kind = server::Command::kRepent;
  server::applyWorldCommand(w, *p, c);
  CHECK(p->karma == 20);
}
