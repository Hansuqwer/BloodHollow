// T-073 gate guards + spawn-camp protection: the gate law has teeth.
// Era pins: Gate Guard (1011, L15, hp 400, dmg 30, def 18, xp 0, aggro 0,
// wander 0, leash 12, guard flag) acquires ONLY wanted players inside leash
// reach; unlawful PK within 8 tiles of a guard anchor marks wanted for 240 s
// (4800 ticks); while wanted, vendors refuse and death binds at the gallows;
// fresh/respawned players are invisible to mob lookup for 5 s (100 ticks).
// Retaliation on being struck still fires (no free hits); the fence asks no
// questions (wanted pawns fine — Sable's lane, flagged).
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
#include "content/items.h"
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

const content::MobDef* guardDef() {
  const content::MobDef* d = content::findMob(1011);
  return d;
}
}  // namespace

TEST_CASE("T-073: guard row pins (L15 wall of duty, xp 0, guard flag)") {
  const content::MobDef* d = guardDef();
  REQUIRE(d != nullptr);
  CHECK(d->level == 15);
  CHECK(d->hp == 400u);
  CHECK(d->dmg == 30u);
  CHECK(d->def == 18u);
  CHECK(d->xp == 0u);
  CHECK(d->aggroRadius == 0u);
  CHECK(d->wanderRadius == 0u);
  CHECK(d->leashRadius == 12u);
  CHECK(d->guard == 1u);
}

TEST_CASE("T-073: unlawful PK at the post marks wanted for 4800 ticks") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& g = w.debugSpawnMob(*guardDef(), sim::TilePos{10, 10});
  server::Entity* killer = spawnP(w, "hothead", 12, 10);  // d=2 from anchor
  server::Entity* victim = spawnP(w, "victim", 13, 10);
  const std::uint32_t kid = killer->id;
  const sim::Tick t0 = w.tickCount();
  w.debugKillPlayerBy(*victim, killer);
  CHECK(w.find(kid)->wantedUntil == t0 + 4800);
  (void)g;
}

TEST_CASE("T-073: far-away murder stays unwatched (no post in 8)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  (void)w.debugSpawnMob(*guardDef(), sim::TilePos{10, 10});
  server::Entity* killer = spawnP(w, "farhot", 30, 30);
  server::Entity* victim = spawnP(w, "farvic", 31, 30);
  w.debugKillPlayerBy(*victim, killer);
  CHECK(w.find(killer->id)->wantedUntil == -1);
}

TEST_CASE("T-073: guards ignore the innocent, take the wanted") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& g = w.debugSpawnMob(*guardDef(), sim::TilePos{10, 10});
  const std::uint32_t gid = g.id;
  server::Entity* clean = spawnP(w, "clean", 12, 10);  // d=2, inside leash
  for (int i = 0; i < 15; ++i) w.tick();
  CHECK(w.find(gid)->attackTarget == 0u);  // the post ignores the innocent
  // the same boots, now wanted: the post answers
  w.find(clean->id)->wantedUntil = w.tickCount() + 4800;
  // past the 100t spawn protection first
  w.debugSetTick(w.tickCount() + 200);
  for (int i = 0; i < 15; ++i) w.tick();
  CHECK(w.find(gid)->attackTarget == clean->id);
  (void)clean;
}

TEST_CASE("T-073: wanted expires on the tick — guards stand down, Marta talks") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& g = w.debugSpawnMob(*guardDef(), sim::TilePos{10, 10});
  const std::uint32_t gid = g.id;
  server::Entity* p = spawnP(w, "timed", 12, 10);
  p->wantedUntil = w.tickCount() + 4800;
  p->gold = 500;
  w.debugSetTick(w.tickCount() + 200);  // past spawn protection
  for (int i = 0; i < 15; ++i) w.tick();
  REQUIRE(w.find(gid)->attackTarget == p->id);  // wanted: hunted
  w.debugSetTick(p->wantedUntil);               // expiry edge
  for (int i = 0; i < 5; ++i) w.tick();
  // guard drops the cold trail (leash/target hygiene on next think)
  bool dropped = w.find(gid)->attackTarget != p->id;
  for (int i = 0; i < 20 && !dropped; ++i) {
    w.tick();
    dropped = w.find(gid)->attackTarget != p->id;
  }
  CHECK(dropped);
}

TEST_CASE("T-073: vendors refuse the wanted (Marta's rage line, extended)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* vend = nullptr;
  for (const auto& e : w.entities())
    if (e.wireKind == content::kWireKindVendor) {
      vend = w.find(e.id);
      break;
    }
  REQUIRE(vend != nullptr);
  server::Entity* p = spawnP(w, "wantedshop", vend->walker.tile().x + 1,
                             vend->walker.tile().y);
  p->gold = 500;
  p->wantedUntil = w.tickCount() + 4800;
  REQUIRE(w.debugGive(*p, 4001, 2));
  REQUIRE(w.debugGive(*p, 2001, 1));  // dented blade for the repair lane
  w.find(p->id)->inv.back().durability = 50;
  CHECK_FALSE(w.vendorBuy(*p, 2001, 1));  // refused like the red
  CHECK(w.vendorSellJunk(*p) == 0u);
  CHECK(p->gold == 500u);
  CHECK_FALSE(w.repairAll(*p));
  // expiry restores the lane
  w.debugSetTick(p->wantedUntil + 1);
  CHECK(w.vendorBuy(*w.find(p->id), 2001, 1));
}

TEST_CASE("T-073: death while wanted binds at the gallows (clean karma)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "gallowsbound", 20, 20);
  p->karma = 0;  // clean — yet the post remembers
  p->wantedUntil = w.tickCount() + 4800;
  const std::uint32_t pid = p->id;
  w.debugKillPlayer(*w.find(pid));
  w.debugSetTick(w.tickCount() + 61);  // past the 60t respawn
  w.tick();
  REQUIRE(w.find(pid)->dead == false);
  CHECK(w.find(pid)->walker.tile() == w.gallowsTile(1));
  CHECK(w.find(pid)->zoneId == 1);
}

TEST_CASE("T-073: spawn protection hides the newborn for exactly 100 ticks") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const content::MobDef* ghoul = content::findMob(1002);  // aggro 6
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{10, 10});
  const std::uint32_t mid = m.id;
  server::Entity* p = spawnP(w, "newborn", 12, 10);  // d=2, inside aggro
  CHECK(p->spawnProtectUntil == w.tickCount() + 100);
  for (int i = 0; i < 15; ++i) w.tick();
  CHECK(w.find(mid)->attackTarget == 0u);  // unseen for 5 s
  w.debugSetTick(w.tickCount() + 200);     // protection long past
  for (int i = 0; i < 15; ++i) w.tick();
  CHECK(w.find(mid)->attackTarget == p->id);  // then the wild sees you
}
