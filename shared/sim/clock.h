#pragma once

#include <cmath>

#include "sim/tick.h"

namespace bh::sim {

// Game clock (GDD section 9): one full day = 4 real hours.
// 20 ticks/s * 600 s = 12000 ticks per game hour.
inline constexpr Tick kTicksPerGameHour = static_cast<Tick>(kTickHz) * 60 * 10;
inline constexpr float kStartHour = 8.0f;  // fresh worlds begin mid-morning

inline float hourAt(Tick worldTick) {
  const double cycle = 24.0 * static_cast<double>(kTicksPerGameHour);
  const double t = std::fmod(static_cast<double>(worldTick), cycle);
  const double h = std::fmod(static_cast<double>(kStartHour) +
                                 t / static_cast<double>(kTicksPerGameHour),
                             24.0);
  return static_cast<float>(h);
}

}  // namespace bh::sim
