// T-132 siege gates: spawn + breach-by-channel (Phase S, 2a/4).
// Pins: spawn shape, gating matrix, progress math, kill + broadcast,
// journaled path.
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

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

server::Entity* spawnP(server::World& w, const char* name, int x, int y,
                       std::uint16_t zone = 1) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y}, zone);
  return w.find(e.id);
}

// battle-ready attacker standing by the Outer Gate (spawn+3,0 of zone 6).
struct SiegeCtx {
  server::Entity* a = nullptr;
  server::Entity* gate = nullptr;
};
SiegeCtx ready(server::World& w) {
  SiegeCtx ctx;
  ctx.a = spawnP(w, "rammer", 5, 5, 6);
  REQUIRE(w.siegeRegister(ctx.a->id));
  w.debugSetTick(1872000);  // Saturday 20:00
  REQUIRE(w.siegeStart(*ctx.a));
  // park by the first gate found in zone 6
  for (const auto& e : w.entities()) {
    if (e.wireKind == content::kWireKindSiegeGate && e.zoneId == 6) {
      ctx.gate = w.find(e.id);
      break;
    }
  }
  REQUIRE(ctx.gate != nullptr);
  const sim::TilePos g = ctx.gate->walker.tile();
  ctx.a->walker.place(sim::TilePos{g.x + 1, g.y});
  return ctx;
}
}  // namespace

TEST_CASE("T-132: zone-6 load seeds exactly 2 standing gates") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  REQUIRE(w.loadZoneFrom(6, makeArena()));
  std::uint32_t gates = 0;
  for (const auto& e : w.entities()) {
    if (e.wireKind != content::kWireKindSiegeGate) continue;
    ++gates;
    CHECK(e.zoneId == 6u);
    CHECK(e.hp == 300u);
    CHECK(e.hpMax == 300u);
    CHECK_FALSE(e.dead);
  }
  CHECK(gates == 2u);
}

TEST_CASE("T-132: breach gating matrix") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  REQUIRE(w.loadZoneFrom(6, makeArena()));
  server::Entity* a = spawnP(w, "rammer", 5, 5, 6);
  REQUIRE(w.siegeRegister(a->id));
  CHECK_FALSE(w.breach(*a));  // no battle yet: quiet
  w.debugSetTick(1872000);
  REQUIRE(w.siegeStart(*a));
  // find a gate and stand far
  server::Entity* gate = nullptr;
  for (const auto& e : w.entities()) {
    if (e.wireKind == content::kWireKindSiegeGate && e.zoneId == 6) {
      gate = w.find(e.id);
      break;
    }
  }
  REQUIRE(gate != nullptr);
  a->walker.place(sim::TilePos{0, 0});
  gate->walker.place(sim::TilePos{39, 39});
  CHECK_FALSE(w.breach(*a));  // out of reach
  // unregistered spectator near the gate
  server::Entity* s = spawnP(w, "spectator", 38, 39, 6);
  CHECK_FALSE(w.breach(*s));
  // in reach: the ram strikes home
  const sim::TilePos g = gate->walker.tile();
  a->walker.place(sim::TilePos{g.x + 1, g.y});
  CHECK(w.breach(*a));
  CHECK(w.find(gate->id)->hp == 290u);
  a->dead = true;
  CHECK_FALSE(w.breach(*a));  // the dead breach nothing
  a->dead = false;
}

TEST_CASE("T-132: thirty rams fell a gate with broadcast") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  REQUIRE(w.loadZoneFrom(6, makeArena()));
  SiegeCtx ctx = ready(w);
  const std::uint32_t gid = ctx.gate->id;
  for (int i = 0; i < 29; ++i) REQUIRE(w.breach(*ctx.a));
  CHECK(w.find(gid)->hp == 10u);
  REQUIRE(w.breach(*ctx.a));  // the 30th: splinters
  CHECK(w.find(gid) == nullptr);
  bool herald = false;
  for (const auto& ev : w.events()) {
    if (ev.chatCh == 2 &&
        ev.chatText.find("lies in splinters!") != std::string::npos)
      herald = true;
  }
  CHECK(herald);
}

TEST_CASE("T-132: journaled breach path") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  REQUIRE(w.loadZoneFrom(6, makeArena()));
  SiegeCtx ctx = ready(w);
  server::Command c;
  c.kind = server::Command::kBreach;
  server::applyWorldCommand(w, *ctx.a, c);
  CHECK(w.find(ctx.gate->id)->hp == 290u);
}
