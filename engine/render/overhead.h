#pragma once

#include <cstdint>

// T-ART-07 overhead name-tint priority (render-side, raylib-free so the unit
// suite can pin it): chaotic red > party green > lawful blue > neutral.
// Enemy-town rank (bible R-TEAM) needs war state, which does not exist and
// no wire carries — there is deliberately no enumerator for it. The call
// site maps these to colors; own-cream is handled before this (isOwn short
// circuits — your own name never competes).
namespace bh {

enum class NameTint : std::uint8_t { kChaotic = 0, kParty, kLawful, kNeutral };

inline NameTint resolveNameTint(bool isOwn, std::uint8_t karmaBand, bool isParty) {
  if (isOwn) return NameTint::kNeutral;  // call site paints own-cream first
  if (karmaBand == 2) return NameTint::kChaotic;
  if (isParty) return NameTint::kParty;
  if (karmaBand == 0) return NameTint::kLawful;
  return NameTint::kNeutral;
}

// R-TEXT-2 crowd degrade (B0 ruling, T-ART-13): the crowd15 failure was
// name-tag pile-up, not silhouettes. When more than kNamePileLimit tags
// overlap, they degrade to the karma-badge glyph only (era HB read — names
// on hover/target — while the PvP karma signal survives).
inline constexpr int kNamePileLimit = 3;
inline constexpr float kNameTagOverlapX = 48.0f;
inline constexpr float kNameTagOverlapY = 16.0f;

inline bool nameTagsOverlap(float ax, float ay, float bx, float by) {
  const float dx = ax > bx ? ax - bx : bx - ax;
  const float dy = ay > by ? ay - by : by - ay;
  return dx < kNameTagOverlapX && dy < kNameTagOverlapY;
}

// overlappingOthers = other tags overlapping this one. A pile of 4+
// (self + 3) degrades; a pair or trio still reads.
inline bool namePileDegrades(int overlappingOthers) {
  return overlappingOthers >= kNamePileLimit;
}

}  // namespace bh
