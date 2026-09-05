#pragma once

#include <cstdint>

namespace bh::sim {

// Authoritative simulation rate for the whole game (ADR-002).
inline constexpr int kTickHz = 20;
inline constexpr double kTickSeconds = 1.0 / kTickHz;

using Tick = std::int64_t;

inline Tick secondsToTicks(double s) { return static_cast<Tick>(s * kTickHz + 0.5); }

}  // namespace bh::sim
