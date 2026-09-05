#pragma once

#include <cstdint>

namespace bh::content {

// Weapon aura tiers (RFC 0001, Soma adoption). Sword-family effects v1; other
// families clone the shape. Failure law: tier III+ may damage, IV/V may destroy.
struct AuraTier {
  std::uint8_t tier;          // 1..5
  std::uint8_t reqSkill;      // weapon skill gate
  std::uint32_t partItemId;   // monster-part currency
  std::uint16_t partQty;
  std::uint32_t gold;
  // effects (v1 sword-family encoding, applied by combat/regen)
  std::uint8_t atkBonusFlat;  // tier I
  std::uint8_t regenPct60t;   // tier II (+N% hpMax per 60 ticks)
  std::uint16_t procId;       // tier III/IV/V hooks (0 for I/II; proc sim lands S10)
  // failure law: -1 = safe, 0 = soft penalty (eat parts&gold), 1 = destroy item
  std::int8_t failLaw;
  std::uint8_t failPct;       // on non-mercy attempts
};

inline constexpr AuraTier kAuraTiers[] = {
    {1, 20, 4001, 30, 120, 3, 0, 0, -1, 0},
    {2, 50, 4002, 15, 400, 0, 2, 0, -1, 0},
    {3, 80, 4004, 10, 1200, 0, 0, 1, 0, 35},
    {4, 120, 4005, 5, 4000, 0, 0, 2, 1, 50},
    {5, 150, 4005, 3, 8000, 0, 0, 3, 1, 35},
};

inline const AuraTier* findAuraTier(std::uint8_t tier) {
  for (const AuraTier& t : kAuraTiers) {
    if (t.tier == tier) return &t;
  }
  return nullptr;
}

}  // namespace bh::content
