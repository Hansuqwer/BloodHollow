// T-ART-04 combat anim hook: pure selection law (render-side, raylib-free).
// The draw call starts state clocks at frame 0 on the pulse tick and falls
// back to walk/idle when the atlas predates combat frames; this file pins
// the selection table, the durations, and the die-hold rule.
#include <doctest/doctest.h>

#include <string>

#include "render/animstate.h"

using namespace bh;

TEST_CASE("T-ART-04: anim selection table (die > hurt > cast/attack)") {
  CHECK(animNameFor(EntAnimState::kAttack, false) == std::string("attack"));
  CHECK(animNameFor(EntAnimState::kCast, false) == std::string("cast"));
  CHECK(animNameFor(EntAnimState::kHurt, true) == std::string("hurt"));
  CHECK(animNameFor(EntAnimState::kDie, false) == std::string("die"));
  CHECK(animNameFor(EntAnimState::kNone, true) == std::string("walk"));
  CHECK(animNameFor(EntAnimState::kNone, false) == std::string("idle"));
}

TEST_CASE("T-ART-04: transient durations pin (die holds, not timed)") {
  CHECK(kAttackAnimTicks == 8);  // half the 16t player swing cadence
  CHECK(kCastAnimTicks == 8);
  CHECK(kHurtAnimTicks == 4);  // a flinch, not a lock
  CHECK(animStateDurationTicks(EntAnimState::kAttack) == 8);
  CHECK(animStateDurationTicks(EntAnimState::kHurt) == 4);
  CHECK(animStateDurationTicks(EntAnimState::kDie) == -1);  // held till despawn
  CHECK(animStateDurationTicks(EntAnimState::kNone) == 0);
}
