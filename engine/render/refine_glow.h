#pragma once

#include <cstdint>

// T-ART-11 refine glow composite (render-side, raylib-free so the unit suite
// can pin it): items glow from +5 (GDD section 7). The composite overlay
// alpha stays at or under 90 (art-backlog cap) so stacked glows never white
// out; +10 is a silhouette change, not a brighter glow (same alpha, new
// marker). The refine ceiling is +7 (T-079), so the mythic branch is
// future-proofing for a later ceiling raise — it is pinned, not reachable.
// In-world sprite overlays need per-entity gear on the wire (no snapshot
// carries refine — deliberate non-wire scope); the call sites glow the
// inventory rows (incl. equipped = in-hand) until that future card.
namespace bh {

enum class RefineGlow : std::uint8_t { kNone = 0, kGlow, kMythic };

inline RefineGlow refineGlowTier(std::uint8_t refine) {
  if (refine >= 10) return RefineGlow::kMythic;
  if (refine >= 5) return RefineGlow::kGlow;
  return RefineGlow::kNone;
}

// Overlay alpha for the glow composite (0 = no overlay).
inline int refineGlowAlpha(std::uint8_t refine) {
  switch (refineGlowTier(refine)) {
    case RefineGlow::kGlow:
    case RefineGlow::kMythic:
      return 70;
    case RefineGlow::kNone:
      return 0;
  }
  return 0;
}

}  // namespace bh
