// T-071 night light: torch + Blessed Lantern + night-only spawns.
// Era pins: torch (3003) grants lightRadius 6 for 300 s (6000 ticks, exact
// edge); lantern (3004) toggles 8 and never expires while held (and is never
// consumed); the mask peaks at kLightGlowAlpha 90 with linear falloff and can
// only lighten (floor-safe by construction); nightOnly spawners refill and
// acquire only inside 21:00–05:00. Light rides the journaled kUseItem lane,
// so live and replay share the branch (no new command — the brief's kUse
// premise was stale; kUseItem is already journaled).
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
#include "content/items.h"
#include "render/lightmask.h"
#include "sim/bhmap.h"
#include "sim/clock.h"
#include "world.h"

using namespace bh;

namespace {
// hours -> ticks from world tick 0 (fresh worlds start at 08:00)
sim::Tick tickAtHour(float h) {
  const double fromStart = h - bh::sim::kStartHour;
  const double wrapped = fromStart < 0 ? fromStart + 24.0 : fromStart;
  return static_cast<sim::Tick>(wrapped * static_cast<double>(bh::sim::kTicksPerGameHour));
}

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

sim::Map makeNightArena() {
  sim::Map m = makeArena();
  sim::SpawnDef sp;
  sp.x = 5;
  sp.y = 5;
  sp.w = 4;
  sp.h = 4;
  sp.mobId = 1002;  // Feral Ghoul: aggro 6, hunts when awake
  sp.maxAlive = 2;
  sp.respawnTicks = 20;
  sp.nightOnly = 1;
  m.spawners.push_back(sp);
  return m;
}

server::Entity* spawnP(server::World& w, const char* name, int x, int y) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y});
  return w.find(e.id);
}

std::uint8_t torchSlot(server::Entity* p) {
  for (std::uint8_t s = 0; s < p->inv.size(); ++s)
    if (p->inv[s].itemId == 3003) return s;
  return 255;
}
}  // namespace

TEST_CASE("T-071: torches grant 6 tiles for 300 s, out at the exact edge") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "torchbearer", 20, 20);
  REQUIRE(w.debugGive(*p, 3003, 2));
  const sim::Tick t0 = w.tickCount();
  REQUIRE(w.useItem(*p, torchSlot(p)));
  CHECK(p->lightRadius == 6u);
  CHECK(p->lightUntil == t0 + 6000);
  CHECK(p->lanternLit == false);
  // one torch burned; the second waits in the pack
  std::uint32_t torches = 0;
  for (const auto& sl : p->inv)
    if (sl.itemId == 3003) torches += sl.qty;
  CHECK(torches == 1u);

  w.debugSetTick(t0 + 5998);
  w.tick();  // now t0+5999: still inside the window
  CHECK(w.find(p->id)->lightRadius == 6u);  // still burning inside the edge
  w.debugSetTick(t0 + 6000);
  w.tick();  // now t0+6001: the edge passed on the tick
  CHECK(w.find(p->id)->lightRadius == 0u);  // out, on the tick
  CHECK(w.find(p->id)->lightUntil == -1);
}

TEST_CASE("T-071: the lantern toggles 8 and never burns down") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "lanternkeeper", 20, 20);
  REQUIRE(w.debugGive(*p, 3004, 1));
  REQUIRE(w.useItem(*p, 0));
  CHECK(p->lightRadius == 8u);
  CHECK(p->lanternLit == true);
  CHECK(p->lightUntil == -1);
  // never consumed: still in the pack after lighting
  CHECK(p->inv.size() == 1u);
  CHECK(p->inv[0].itemId == 3004u);
  // a full day-night cycle later it still burns
  w.debugSetTick(w.tickCount() + 100000);
  w.tick();
  CHECK(w.find(p->id)->lightRadius == 8u);
  // shutter it again through the same lane
  for (int i = 0; i < 20; ++i) w.tick();  // past the sip gate
  REQUIRE(w.useItem(*w.find(p->id), 0));
  CHECK(w.find(p->id)->lightRadius == 0u);
  CHECK(w.find(p->id)->lanternLit == false);
}

TEST_CASE("T-071: mask law pins — peak 90, linear falloff, dark at zero") {
  CHECK(bh::kLightGlowAlpha == 90.0f);
  CHECK(bh::lightGlowFalloff(0.0f, 6.0f) == 1.0f);
  CHECK(bh::lightGlowFalloff(3.0f, 6.0f) == doctest::Approx(0.5f));
  CHECK(bh::lightGlowFalloff(6.0f, 6.0f) == 0.0f);
  CHECK(bh::lightGlowFalloff(9.0f, 6.0f) == 0.0f);
  CHECK(bh::lightGlowFalloff(0.0f, 0.0f) == 0.0f);  // radius 0 = dark
  CHECK(bh::lightGlowAlpha(0.0f, 6.0f) == 90);
  CHECK(bh::lightGlowAlpha(6.0f, 6.0f) == 0);
  CHECK(bh::kLightGlowAlpha < 150.0f);  // overlapping pools stay under the floor
}

TEST_CASE("T-071: nightOnly refill gating at the hour edge") {
  server::World w;
  REQUIRE(w.loadFrom(makeNightArena()));  // boot is 08:00: day
  auto ghouls = [&]() {
    std::size_t n = 0;
    for (const auto& e : w.entities())
      if (e.kind == server::EntityKind::kMob && e.mobId == 1002) ++n;
    return n;
  };
  CHECK(ghouls() == 0u);  // no initial spawn by day
  for (int i = 0; i < 10; ++i) w.tick();
  CHECK(ghouls() == 0u);  // refill gated too
  // dusk settles: the rect fills (one per spawner per 20t refill check)
  w.debugSetTick(tickAtHour(21.5f));
  for (int i = 0; i < 30; ++i) w.tick();
  CHECK(ghouls() == 2u);
}

TEST_CASE("T-071: nightOnly aggro gating — seen at night, ignored by day") {
  server::World w;
  REQUIRE(w.loadFrom(makeNightArena()));
  w.debugSetTick(tickAtHour(21.5f));
  for (int i = 0; i < 10; ++i) w.tick();  // night fill
  server::Entity* mob = nullptr;
  for (auto& e : w.entities())
    if (e.kind == server::EntityKind::kMob && e.mobId == 1002) {
      mob = &e;
      break;
    }
  REQUIRE(mob != nullptr);
  const sim::TilePos den = mob->walker.tile();
  server::Entity* p = spawnP(w, "nightwalk", den.x + 2, den.y);  // d=2, in aggro
  const std::uint32_t pid = p->id;
  const std::uint32_t mid = mob->id;
  for (int i = 0; i < 15; ++i) w.tick();
  CHECK(w.find(mid)->attackTarget == pid);  // night eyes acquire
  // dawn breaks on a fresh world with the same shape: no acquire
  server::World w2;
  REQUIRE(w2.loadFrom(makeNightArena()));
  w2.debugSetTick(tickAtHour(21.5f));
  for (int i = 0; i < 10; ++i) w2.tick();
  server::Entity* mob2 = nullptr;
  for (auto& e : w2.entities())
    if (e.kind == server::EntityKind::kMob && e.mobId == 1002) {
      mob2 = &e;
      break;
    }
  REQUIRE(mob2 != nullptr);
  const sim::TilePos den2 = mob2->walker.tile();
  const std::uint32_t mid2 = mob2->id;
  (void)spawnP(w2, "daywalk", den2.x + 2, den2.y);
  w2.debugSetTick(tickAtHour(10.0f));  // day: the pack sleeps
  for (int i = 0; i < 15; ++i) w2.tick();
  CHECK(w2.find(mid2)->attackTarget == 0u);
}

TEST_CASE("T-071: SpawnDef nightOnly defaults off (day content unchanged)") {
  sim::SpawnDef s;
  CHECK(s.nightOnly == 0u);
}

TEST_CASE("T-071: torch use through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "replaytorch", 20, 20);
  REQUIRE(w.debugGive(*p, 3003, 1));
  server::Command c;
  c.kind = server::Command::kUseItem;
  c.channel = torchSlot(p);
  server::applyWorldCommand(w, *p, c);
  CHECK(p->lightRadius == 6u);
}
