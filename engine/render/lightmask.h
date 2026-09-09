#pragma once

// T-071 night light: carried-light mask law (render-side, raylib-free so the
// unit suite can pin it). At night the world wears nightOverlay() up to the
// T-062 legibility floor (alpha 150 — horror comes from content, not
// darkness); lit tiles are painted back ADDITIVELY, so the mask can only
// ever lighten, never push below the floor.
//
// A source of radius R tiles glows with a linear falloff: full strength at
// the holder, zero at R. Strength scales the warm tint alpha (kGlowAlpha,
// well under the overlay floor so overlapping pools never white out).
namespace bh {

inline constexpr float kLightGlowAlpha = 90.0f;  // peak warm tint at the holder
inline constexpr float kLightTilePx = 48.0f;     // glow radius per light tile (screen px, iso avg)

// Linear falloff 1.0 -> 0.0 across [0, radiusTiles]; 0 outside. radius 0 = dark.
inline float lightGlowFalloff(float distTiles, float radiusTiles) {
  if (radiusTiles <= 0.0f || distTiles < 0.0f) return 0.0f;
  if (distTiles >= radiusTiles) return 0.0f;
  return 1.0f - distTiles / radiusTiles;
}

// Alpha for a screen point: peak * falloff, rounded down (era-plain).
inline int lightGlowAlpha(float distTiles, float radiusTiles) {
  return static_cast<int>(kLightGlowAlpha * lightGlowFalloff(distTiles, radiusTiles));
}

}  // namespace bh
