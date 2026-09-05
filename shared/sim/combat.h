#pragma once

#include <cstdint>

#include "sim/rng.h"

namespace bh::sim {

// Combat resolution (GDD docs/02 section 4). Deterministic: integer math only,
// all randomness through sim::Rng. Shared by server (authoritative) and tests.
// Tuning constants live here so balance changes are one-diff auditable.

// XP to go from level L to L+1: round_to_10(100 * L^1.85). Precomputed
// literals (NOT powf at runtime) so the curve is identical on every platform
// forever. Index 0 unused; valid levels 1..25 (MVP cap).
inline constexpr std::uint32_t kXpNext[26] = {
    0,     100,   360,   760,   1300,  1960,  2750,  3660,  4690, 5830,
    7080,  8440,  9920,  11500, 13190, 14990, 16890, 18890, 21000, 23210,
    25520, 27930, 30440, 33050, 35760, 38560};

inline constexpr std::uint8_t kLevelCap = 25;
inline constexpr std::uint8_t kStatPointsPerLevel = 3;

inline std::uint32_t xpNext(std::uint8_t level) {
  if (level < 1) return kXpNext[1];
  if (level >= kLevelCap) return 0;  // capped: no next bar
  return kXpNext[level];
}

// Player max HP: base + level growth + VIT. Weapon base damage folds fists.
constexpr std::uint32_t playerHpMax(std::uint8_t level, std::uint8_t vit) {
  return 40u + 6u * level + 6u * vit;
}

// GDD hit model: clamp(55 + (ACC - EVD)*1, 5, 98) with ACC=2*DEX, EVD=DEX.
// All inputs are rates in whole percent; range checks the GDD clamps.
struct HitCheck {
  bool hit = false;
  bool crit = false;
  int hitChancePct = 0;  // exposed for tests + future client readability
};

inline HitCheck rollHit(int acc, int evd, int attackerDex, Rng& rng) {
  HitCheck r;
  int pct = 55 + (acc - evd);
  pct = pct < 5 ? 5 : (pct > 98 ? 98 : pct);
  r.hitChancePct = pct;
  r.hit = rng.range(1, 100) <= pct;
  if (r.hit) {
    // crit: 5% base, +1% per 10 DEX above 30
    int critPct = 5 + (attackerDex > 30 ? (attackerDex - 30) / 10 : 0);
    r.crit = rng.range(1, 100) <= critPct;
  }
  return r;
}

// GDD damage: raw = base*(1 + STR*0.02); dmg = raw * 100/(100+DEF).
// Integerized as permille to stay deterministic; crits x170%.
inline std::uint32_t rollDamage(std::uint32_t baseWeapon, std::uint8_t str,
                                std::uint32_t def, bool crit) {
  const std::uint32_t raw = baseWeapon * (100u + 2u * str) / 100u;
  std::uint32_t dmg = raw * 100u / (100u + def);
  if (crit) dmg = dmg * 170u / 100u;
  return dmg < 1 ? 1 : dmg;  // chip damage always possible
}

}  // namespace bh::sim
