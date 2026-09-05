#include <doctest/doctest.h>

#include "sim/clock.h"

using namespace bh;

TEST_CASE("clock: start hour and game-hour conversion") {
  CHECK(sim::hourAt(0) == doctest::Approx(sim::kStartHour));
  CHECK(sim::hourAt(sim::kTicksPerGameHour) == doctest::Approx(sim::kStartHour + 1.0));
  // 2.5 game-hours of night transition arithmetic, 18:30:
  const sim::Tick t = static_cast<sim::Tick>(10.5 * sim::kTicksPerGameHour);
  CHECK(sim::hourAt(t) == doctest::Approx(18.5));
}

TEST_CASE("clock: wraps every 24 game-hours, never negative") {
  const sim::Tick day = 24 * sim::kTicksPerGameHour;
  CHECK(sim::hourAt(day) == doctest::Approx(sim::hourAt(0)));
  CHECK(sim::hourAt(3 * day + sim::kTicksPerGameHour) ==
        doctest::Approx(sim::hourAt(sim::kTicksPerGameHour)));
}
