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

}  // namespace bh
