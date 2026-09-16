#pragma once

#include <cstdint>

namespace bh::content {

// Town war (GDD section 1; T-130): at level 19 you swear to one town to keep
// leveling (Helbreath rule). Killing a sworn member of the enemy town grants
// EK fame instead of chaos. One oath, no respec in MVP.
inline constexpr std::uint8_t kTownNone = 0;
inline constexpr std::uint8_t kTownThornwall = 1;   // Ashen Compact (west)
inline constexpr std::uint8_t kTownMarrowgate = 2;  // Pale Synod (east)
inline constexpr std::uint8_t kTownOathLevel = 19;

inline constexpr const char* kTownNames[] = {"unsworn", "Thornwall", "Marrowgate"};
inline constexpr const char* kTownFactions[] = {"", "Ashen Compact", "Pale Synod"};

}  // namespace bh::content
