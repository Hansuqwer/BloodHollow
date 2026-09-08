#pragma once

#include <cstdint>

namespace bh::content {

// Entity wire-render kinds: mobs are mobId-family 0..62; 63+ is RESERVED for
// furniture/non-combat entities (vendor=64, anvil=65). Filters MUST use these
// bounds instead of magic numbers (T-047): mobs < floor, furniture >= floor.
inline constexpr std::uint8_t kWireKindFurnitureFloor = 64;
inline constexpr std::uint8_t kWireKindVendor = 64;
inline constexpr std::uint8_t kWireKindAnvil = 65;
inline constexpr std::uint8_t kWireKindBounty = 66;  // T-065 wanted board
// 67 Bonesmith twins / 68 confessor: RESERVED (art REGISTRY, T-ART-06).
inline constexpr std::uint8_t kWireKindFence = 69;   // T-069 Smugglers' Cove fence

inline bool wireIsFurniture(std::uint8_t k) { return k >= kWireKindFurnitureFloor; }
inline bool wireIsMob(std::uint8_t k) { return k < kWireKindFurnitureFloor; }

}  // namespace bh::content
