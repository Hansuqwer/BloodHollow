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
// 67 Bonesmith twins: RESERVED (art REGISTRY, T-ART-06).
inline constexpr std::uint8_t kWireKindBonesmith = 67;  // T-ART-06 twins
inline constexpr std::uint8_t kWireKindConfessor = 68;  // T-070 chapel cure
inline constexpr std::uint8_t kWireKindFence = 69;   // T-069 Smugglers' Cove fence
inline constexpr std::uint8_t kWireKindGuardAshen = 70;  // T-ART-06 town guard
inline constexpr std::uint8_t kWireKindGuardSynod = 71;  // T-ART-06 town guard
inline constexpr std::uint8_t kWireKindRegistrar = 72;   // T-ART-06 pledge clerk
inline constexpr std::uint8_t kWireKindSteward = 73;     // T-ART-06 castle keeper
inline constexpr std::uint8_t kWireKindCastleGate = 74;   // T-123 siege gate (100k HP)
inline constexpr std::uint8_t kWireKindHeartstone = 75;   // T-123 courtyard heart
inline constexpr std::uint8_t kWireKindThrone = 76;       // T-123 crown-channel seat

inline bool wireIsFurniture(std::uint8_t k) { return k >= kWireKindFurnitureFloor; }
inline bool wireIsMob(std::uint8_t k) { return k < kWireKindFurnitureFloor; }

}  // namespace bh::content
