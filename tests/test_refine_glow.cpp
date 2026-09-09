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
