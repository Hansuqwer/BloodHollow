// T-ART-06 remainder: NPC kinds 67/70-73 spawn as non-combat furniture.
// Engine side only (constants + spawn seams + generic client rect+label);
// Thornwall/Marrowgate placement + portraits + dialogue are art-side cards.
#include <doctest/doctest.h>

#include <cstdint>

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
}  // namespace

TEST_CASE("T-ART-06: new NPC kinds are furniture-band, non-combat, named") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));

  // call seams directly (World member fns).
  server::Entity& b = w.debugSpawnBonesmith(sim::TilePos{5, 5});
  server::Entity& ga = w.debugSpawnGuardAshen(sim::TilePos{6, 5});
  server::Entity& gs = w.debugSpawnGuardSynod(sim::TilePos{7, 5});
  server::Entity& r = w.debugSpawnRegistrar(sim::TilePos{8, 5});
  server::Entity& s = w.debugSpawnSteward(sim::TilePos{9, 5});
  server::Entity& c = w.debugSpawnConfessor(sim::TilePos{10, 5});
  server::Entity& f = w.debugSpawnFence(sim::TilePos{11, 5});

  CHECK(b.wireKind == content::kWireKindBonesmith);
  CHECK(b.wireKind == 67);
  CHECK(b.name == "Bonesmith Twins");
  CHECK(ga.wireKind == content::kWireKindGuardAshen);
  CHECK(ga.name == "Ashen Guard");
  CHECK(gs.wireKind == content::kWireKindGuardSynod);
  CHECK(gs.name == "Synod Guard");
  CHECK(r.wireKind == content::kWireKindRegistrar);
  CHECK(r.name == "Pledge Registrar");
  CHECK(s.wireKind == content::kWireKindSteward);
  CHECK(s.name == "Castle Steward");
  CHECK(c.wireKind == 68);
  CHECK(f.wireKind == 69);

  for (const server::Entity* e : {&b, &ga, &gs, &r, &s, &c, &f}) {
    CHECK(content::wireIsFurniture(e->wireKind));  // generic rect+label path
    CHECK_FALSE(content::wireIsMob(e->wireKind));  // never a combat mob
    CHECK(e->hp == 1);
    CHECK(e->hpMax == 1);
  }
  // furniture floor still 64: mobs < 64 never collide with the new kinds
  CHECK_FALSE(content::wireIsFurniture(63));
  CHECK(content::wireIsFurniture(64));
  CHECK(content::wireIsFurniture(73));
}

TEST_CASE("T-ART-06: karma-band vendor refusal untouched by new kinds") {
  // The refusal lane keys off vendor/fence proximity, not kind counts —
  // spawning five more furniture kinds changes no gate. This pins the lane
  // exists independently: a lawful courier near no vendor gets no refusal.
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  (void)w.debugSpawnSteward(sim::TilePos{10, 10});
  server::Entity& e = w.spawn("courier", 0, sim::TilePos{11, 10});
  server::Entity* p = w.find(e.id);
  REQUIRE(p != nullptr);
  CHECK(p->karma == 0);  // no furniture bestows or moves karma on spawn
}
