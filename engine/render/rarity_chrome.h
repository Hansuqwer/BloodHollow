#pragma once

#include <cstdint>

// T-159f1.3 rarity chrome (render-side, raylib-free so the unit suite can
// pin it): the T-159 rarity tiers (0 common / 1 magic / 2 rare / 3 unique)
// already ride the wire (ItemSlot rarity, messages.md) and sit in the
// client's NetItem — nothing rendered them. This header is the text half
// of the chrome (bag-row marker + presence); the client maps tiers to
// colours at the call site. Out-of-range tiers render as common (never
// trust the wire for a colour).
namespace bh {

inline bool hasRarityChrome(std::uint8_t rarity) {
  return rarity >= 1 && rarity <= 3;
}

inline const char* rarityMarker(std::uint8_t rarity) {
  switch (rarity) {
    case 1:
      return "M ";
    case 2:
      return "R ";
    case 3:
      return "U ";
    default:
      return "";
  }
}

}  // namespace bh
