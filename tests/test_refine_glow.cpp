// T-ART-11 refine glow law: +5 glows (alpha-capped composite), +10 is a
// silhouette change, not brighter. Raylib-free header, display-free pins.
#include <doctest/doctest.h>

#include "render/refine_glow.h"

using namespace bh;

TEST_CASE("T-ART-11: glow tiers ride refine (5 glows, 10 mythic)") {
  CHECK(refineGlowTier(0) == RefineGlow::kNone);
  CHECK(refineGlowTier(4) == RefineGlow::kNone);  // just below the glow
  CHECK(refineGlowTier(5) == RefineGlow::kGlow);  // GDD: glows from +5
  CHECK(refineGlowTier(7) == RefineGlow::kGlow);  // current ceiling (T-079)
  CHECK(refineGlowTier(9) == RefineGlow::kGlow);
  CHECK(refineGlowTier(10) == RefineGlow::kMythic);  // silhouette, not brighter
  CHECK(refineGlowTier(12) == RefineGlow::kMythic);
}

TEST_CASE("T-ART-11: overlay alpha capped at 70 (backlog cap 90)") {
  CHECK(refineGlowAlpha(0) == 0);
  CHECK(refineGlowAlpha(4) == 0);
  CHECK(refineGlowAlpha(5) == 70);
  CHECK(refineGlowAlpha(7) == 70);
  CHECK(refineGlowAlpha(10) == 70);  // mythic: same alpha, new marker
  CHECK(refineGlowAlpha(10) <= 90);
}

// T-092: server-side scan feeds the wire field — equipped weapon refine maps
// to the tier both ends agree on; dormant steel and fists stay dark.
#include "world.h"

namespace {
sim::Map makeGlowArena() {
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

void armBlade(server::Entity& e, std::uint8_t refine, std::uint8_t dura = 100) {
  for (auto& sl : e.inv)
    if (sl.itemId == 2002) {  // Pit Blade (slot 0)
      sl.equipped = true;
      sl.refine = refine;
      sl.durability = dura;
    }
}
}  // namespace

TEST_CASE("T-092: equippedGlowTier mirrors the client law") {
  server::World w;
  REQUIRE(w.loadFrom(makeGlowArena()));
  server::Entity& e = w.spawn("smith", 0, sim::TilePos{5, 5});
  REQUIRE(w.debugGive(e, 2002, 1));
  CHECK(w.equippedGlowTier(e) == 0);  // owned but not armed: dark
  armBlade(e, 4);
  CHECK(w.equippedGlowTier(e) == 0);  // below the glow
  armBlade(e, 5);
  CHECK(w.equippedGlowTier(e) == 1);  // GDD: glows from +5
  armBlade(e, 7);
  CHECK(w.equippedGlowTier(e) == 1);  // current ceiling (T-079)
  armBlade(e, 10);
  CHECK(w.equippedGlowTier(e) == 2);  // mythic branch, future ceiling
  armBlade(e, 7, 0);
  CHECK(w.equippedGlowTier(e) == 0);  // dormant never glows (T-058)

  server::Entity& f = w.spawn("fists", 0, sim::TilePos{6, 5});
  CHECK(w.equippedGlowTier(f) == 0);  // no steel, no glow

  const content::MobDef* rat = content::findMob(1001);
  REQUIRE(rat != nullptr);
  server::Entity& m = w.debugSpawnMob(*rat, sim::TilePos{7, 5});
  CHECK(w.equippedGlowTier(m) == 0);  // mobs carry no gear
}
