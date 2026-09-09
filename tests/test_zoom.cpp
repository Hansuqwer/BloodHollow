// T-ART-02 wheel-zoom snap: art is validated at {1, 1.5, 2}; anything else
// shimmers. snapZoom lives inline in engine/render/camera_rig.h (pure float
// math — the header needs raylib at compile, hence the raylib test link).
#include <doctest/doctest.h>

#include "render/camera_rig.h"

using namespace bh;

TEST_CASE("T-ART-02: wheel zoom snaps to {1, 1.5, 2}") {
  CHECK(snapZoom(1.0f) == 1.0f);
  CHECK(snapZoom(1.24f) == 1.0f);
  CHECK(snapZoom(1.25f) == 1.5f);  // boundary pins from the backlog
  CHECK(snapZoom(1.5f) == 1.5f);
  CHECK(snapZoom(1.74f) == 1.5f);
  CHECK(snapZoom(1.75f) == 2.0f);  // boundary pins from the backlog
  CHECK(snapZoom(2.0f) == 2.0f);
  CHECK(snapZoom(2.5f) == 2.0f);   // old clamp ceiling now snaps to 2
  CHECK(snapZoom(0.5f) == 1.0f);   // old floor now snaps to 1
  CHECK(snapZoom(10.0f) == 2.0f);
}
