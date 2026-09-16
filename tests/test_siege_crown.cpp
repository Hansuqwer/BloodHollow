// T-133 Heartstone capture + crown channel (Phase S, 2b/4).
// Pins: stone spawn, accumulation/contest/hold, attune broadcast, crown
// gates, full channel, move/hit/death/leave breaks, holder + battle end,
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

// battle-ready attacker parked by the Heartstone in zone 6.
struct CrownCtx {
  server::Entity* a = nullptr;
  server::Entity* stone = nullptr;
};
CrownCtx ready(server::World& w, const char* name = "crowner") {
  CrownCtx ctx;
  ctx.a = spawnP(w, name, 5, 5, 6);
  REQUIRE(w.siegeRegister(ctx.a->id));
  w.debugSetTick(1872000);  // Saturday 20:00
  REQUIRE(w.siegeStart(*ctx.a));
  for (const auto& e : w.entities()) {
    if (e.wireKind == content::kWireKindHeartstone && e.zoneId == 6) {
      ctx.stone = w.find(e.id);
      break;
    }
  }
  REQUIRE(ctx.stone != nullptr);
  const sim::TilePos g = ctx.stone->walker.tile();
  ctx.a->walker.place(sim::TilePos{g.x + 1, g.y});
  ctx.a->path.clear();
  return ctx;
}

void attune(server::World& w) {
  for (int t = 0; t < 1200; ++t) w.tick();
}
}  // namespace

TEST_CASE("T-133: zone-6 load seeds a standing Heartstone") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  REQUIRE(w.loadZoneFrom(6, makeArena()));
  std::uint32_t stones = 0;
  for (const auto& e : w.entities()) {
    if (e.wireKind != content::kWireKindHeartstone) continue;
    ++stones;
    CHECK(e.zoneId == 6u);
    CHECK(e.hp == 1u);
  }
  CHECK(stones == 1u);
  CHECK_FALSE(w.heartAttuned());
  CHECK(w.heartProgress() == 0u);
}

TEST_CASE("T-133: attackers attune uncontested, contest freezes") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  REQUIRE(w.loadZoneFrom(6, makeArena()));
  CrownCtx ctx = ready(w);
  for (int t = 0; t < 600; ++t) w.tick();
  CHECK(w.heartProgress() == 600u);
  // a spectator steps into the ring: progress holds
  const sim::TilePos g = ctx.stone->walker.tile();
  server::Entity* s = spawnP(w, "spectator", g.x - 1, g.y, 6);
  (void)s;
  for (int t = 0; t < 100; ++t) w.tick();
  CHECK(w.heartProgress() == 600u);
  // ring clear: attunement completes with broadcast
  w.despawn(s->id);
  for (int t = 0; t < 600; ++t) w.tick();
  CHECK(w.heartProgress() == 1200u);
  CHECK(w.heartAttuned());
  bool herald = false;
  for (const auto& ev : w.events()) {
    if (ev.chatCh == 2 &&
        ev.chatText.find("it is attuned") != std::string::npos)
      herald = true;
  }
  CHECK(herald);
}

TEST_CASE("T-133: crown gates hold before attunement") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  REQUIRE(w.loadZoneFrom(6, makeArena()));
  CrownCtx ctx = ready(w);
  CHECK_FALSE(w.crown(*ctx.a));  // stone cold: quiet
  server::Entity* out = spawnP(w, "outsider", 5, 5, 6);  // unregistered
  const sim::TilePos g = ctx.stone->walker.tile();
  out->walker.place(sim::TilePos{g.x - 1, g.y});
  CHECK_FALSE(w.crown(*out));  // cold AND unenlisted
  out->walker.place(sim::TilePos{0, 0});  // ring clear before attuning
  attune(w);
  REQUIRE(w.heartAttuned());
  out->walker.place(sim::TilePos{g.x - 1, g.y});
  CHECK_FALSE(w.crown(*out));  // attuned but not enlisted
  out->walker.place(sim::TilePos{0, 0});
  ctx.a->walker.place(sim::TilePos{0, 0});
  CHECK_FALSE(w.crown(*ctx.a));  // out of reach
}

TEST_CASE("T-133: a full kneel crowns the holder and ends the battle") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  REQUIRE(w.loadZoneFrom(6, makeArena()));
  CrownCtx ctx = ready(w);
  attune(w);
  REQUIRE(w.crown(*ctx.a));
  for (int t = 0; t < 200; ++t) w.tick();
  CHECK(w.siegeHolder() == ctx.a->id);
  CHECK_FALSE(w.siegeBattleActive());
  bool herald = false;
  for (const auto& ev : w.events()) {
    if (ev.chatCh == 2 &&
        ev.chatText.find("takes the Weeping Crown!") != std::string::npos)
      herald = true;
  }
  CHECK(herald);
}

TEST_CASE("T-133: move, hit, death, and leaving all break the kneel") {
  // move
  {
    server::World w;
    REQUIRE(w.loadFrom(makeArena()));
    REQUIRE(w.loadZoneFrom(6, makeArena()));
    CrownCtx ctx = ready(w);
    attune(w);
    REQUIRE(w.crown(*ctx.a));
    for (int t = 0; t < 50; ++t) w.tick();
    // feet move: a valid adjacent chain (non-adjacent waypoints are
    // correctly dropped by movement validation — see devlog 0094)
    const sim::TilePos feet = ctx.a->walker.tile();
    for (int i = 1; i <= 5; ++i)
      ctx.a->path.push_back(sim::TilePos{feet.x - i, feet.y});
    w.tick();
    CHECK(ctx.a->crownUntil < 0);
    CHECK(w.siegeHolder() == 0u);
  }
  // hit
  {
    server::World w;
    REQUIRE(w.loadFrom(makeArena()));
    REQUIRE(w.loadZoneFrom(6, makeArena()));
    CrownCtx ctx = ready(w);
    attune(w);
    REQUIRE(w.crown(*ctx.a));
    for (int t = 0; t < 50; ++t) w.tick();
    ctx.a->lastHurtTick = w.tickCount();  // steel lands
    w.tick();
    CHECK(ctx.a->crownUntil < 0);
    CHECK(w.siegeHolder() == 0u);
  }
  // death
  {
    server::World w;
    REQUIRE(w.loadFrom(makeArena()));
    REQUIRE(w.loadZoneFrom(6, makeArena()));
    CrownCtx ctx = ready(w);
    attune(w);
    REQUIRE(w.crown(*ctx.a));
    ctx.a->dead = true;
    w.tick();
    CHECK(ctx.a->crownUntil < 0);
    CHECK(w.siegeHolder() == 0u);
  }
  // leaving the radius
  {
    server::World w;
    REQUIRE(w.loadFrom(makeArena()));
    REQUIRE(w.loadZoneFrom(6, makeArena()));
    CrownCtx ctx = ready(w);
    attune(w);
    REQUIRE(w.crown(*ctx.a));
    ctx.a->walker.place(sim::TilePos{0, 0});
    w.tick();
    CHECK(ctx.a->crownUntil < 0);
    CHECK(w.siegeHolder() == 0u);
  }
}

TEST_CASE("T-133: journaled crown path") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  REQUIRE(w.loadZoneFrom(6, makeArena()));
  CrownCtx ctx = ready(w);
  attune(w);
  server::Command c;
  c.kind = server::Command::kCrown;
  server::applyWorldCommand(w, *ctx.a, c);
  CHECK(ctx.a->crownUntil > 0);
}
