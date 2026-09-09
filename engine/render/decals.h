#pragma once

#include <cstdint>

// T-ART-09 ground decal layer (render-side, raylib-free so the unit suite
// can pin it): blood, boss telegraphs, and persistent spell circles live on
// a decal surface drawn UNDER entities, layered by tile row, with decay
// timers. Rendering-only, non-wire: nothing here touches the sim, the
// journal, or worldHash — replay stays bit-exact by construction.
//
// Clocks are client sim-ticks (20 Hz, same basis as the server journal, but
// read here as a render clock only). Blood persists 10 min (12000 ticks,
// backlog section 4.4); telegraphs run a 3-stage cycle (warn 20t / arm 20t /
// strike 40t); circles persist until the zone reloads (Sanctuary/Wither).
namespace bh {

enum class DecalKind : std::uint8_t {
  kBlood = 0,
  kTelegraph1,
  kTelegraph2,
  kTelegraph3,
  kCircle,
};

struct Decal {
  float x = 0;  // tile-space float (victim/caster ground pos)
  float y = 0;
  DecalKind kind = DecalKind::kBlood;
  std::int64_t bornTick = 0;
};

inline constexpr std::int64_t kBloodDecalTtl = 12000;    // 10 min at 20 Hz
inline constexpr std::int64_t kTelegraph1Ttl = 20;       // warn
inline constexpr std::int64_t kTelegraph2Ttl = 20;       // arm
inline constexpr std::int64_t kTelegraph3Ttl = 40;       // strike flash
inline constexpr std::size_t kDecalCap = 512;            // FIFO per surface

inline std::int64_t decalTtl(DecalKind k) {
  switch (k) {
    case DecalKind::kBlood:
      return kBloodDecalTtl;
    case DecalKind::kTelegraph1:
      return kTelegraph1Ttl;
    case DecalKind::kTelegraph2:
      return kTelegraph2Ttl;
    case DecalKind::kTelegraph3:
      return kTelegraph3Ttl;
    case DecalKind::kCircle:
      return -1;  // persistent until zone reload
  }
  return 0;
}

inline bool decalAlive(const Decal& d, std::int64_t nowTick) {
  const std::int64_t ttl = decalTtl(d.kind);
  if (ttl < 0) return true;
  return nowTick - d.bornTick < ttl;
}

// T-091: one wind-up event stages itself — the client advances 1-2-3 by age
// (warn 0-20t, arm 20-40t, strike flash 40-60t) instead of spending three
// journaled events. The server's 60t fuse and this staging share a clock.
// Stage 3 lingers 20t past the strike as scorch, so a wind-up decal stays
// visible 80t total (not the single-stage 20t).
inline constexpr std::int64_t kWindupVisibleTtl =
    kTelegraph1Ttl + kTelegraph2Ttl + kTelegraph3Ttl;

inline int telegraphStage(const Decal& d, std::int64_t nowTick) {
  const std::int64_t age = nowTick - d.bornTick;
  if (age < kTelegraph1Ttl) return 1;
  if (age < kTelegraph1Ttl + kTelegraph2Ttl) return 2;
  return 3;
}

inline bool decalVisible(const Decal& d, std::int64_t nowTick) {
  if (d.kind == DecalKind::kTelegraph1 || d.kind == DecalKind::kTelegraph2 ||
      d.kind == DecalKind::kTelegraph3)
    return nowTick - d.bornTick < kWindupVisibleTtl;
  return decalAlive(d, nowTick);
}

// Linear fade to transparent across the TTL (era-plain); circles hold full.
// Wind-up telegraphs fade across the whole 80t visible window (not the
// single-stage TTL), so the ring survives its own staging.
inline int decalAlpha(const Decal& d, std::int64_t nowTick) {
  if (d.kind == DecalKind::kTelegraph1 || d.kind == DecalKind::kTelegraph2 ||
      d.kind == DecalKind::kTelegraph3) {
    const std::int64_t age = nowTick - d.bornTick;
    if (age < 0) return 150;
    if (age >= kWindupVisibleTtl) return 0;
    return static_cast<int>(150 - 90 * age / kWindupVisibleTtl);
  }
  const std::int64_t ttl = decalTtl(d.kind);
  if (ttl < 0) return 110;
  const std::int64_t age = nowTick - d.bornTick;
  if (age < 0) return 110;
  if (age >= ttl) return 0;
  if (d.kind == DecalKind::kBlood) return static_cast<int>(110 - 70 * age / ttl);
  return static_cast<int>(150 - 90 * age / ttl);
}

}  // namespace bh
