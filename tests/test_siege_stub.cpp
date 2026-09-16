// H1 siege stub: `gm siege-now` registers + activates a 90-min rehearsal.
// Pins: flags flip, fiction line emits, repeat-while-active fails quiet,
// dead callers fail quiet, the journaled path (kSiegeNow) matches the live
// call, and zone 6 boots the Castle Steward (kind 73).
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
#include "content/wirekind.h"
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

TEST_CASE("H1: siege-now registers and activates the rehearsal") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "hornblower", 10, 10);
  CHECK_FALSE(w.siegeActive());
  CHECK_FALSE(w.siegeRegistered());
  REQUIRE(w.siegeNow(*p));
  CHECK(w.siegeRegistered());
  CHECK(w.siegeActive());
  CHECK(w.siegeEndsAt() == w.tickCount() + 108000);  // 90 min at 20 Hz
  // fiction line on channel 2
  bool horn = false;
  for (const auto& ev : w.events())
    if (ev.chatCh == 2 && ev.chatText.find("war-horn") != std::string::npos)
      horn = true;
  CHECK(horn);
}

TEST_CASE("H1: one rehearsal at a time — repeats fail quiet") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "first", 10, 10);
  server::Entity* q = spawnP(w, "second", 12, 12);
  REQUIRE(w.siegeNow(*p));
  CHECK_FALSE(w.siegeNow(*q));
  CHECK(w.siegeEndsAt() == w.tickCount() + 108000);  // not refreshed
}

TEST_CASE("H1: the dead sound no horn") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "fallen", 10, 10);
  p->dead = true;
  CHECK_FALSE(w.siegeNow(*p));
  CHECK_FALSE(w.siegeActive());
}

TEST_CASE("H1: gm siege-now through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "roundtrip", 10, 10);
  server::Command c;
  c.kind = server::Command::kSiegeNow;
  server::applyWorldCommand(w, *p, c);
  CHECK(w.siegeActive());
  CHECK(w.siegeRegistered());
}

TEST_CASE("H1: castle zone boots the steward (kind 73)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  sim::Map castle = makeArena();
  REQUIRE(w.loadZoneFrom(6, std::move(castle)));
  bool steward = false;
  for (const auto& e : w.entities())
    if (e.zoneId == 6 && e.wireKind == content::kWireKindSteward &&
        e.name == "Castle Steward")
      steward = true;
  CHECK(steward);
}
