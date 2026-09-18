#pragma once

// T-ART-14 VFX strip law (raylib-free so the unit suite can pin it).
// VFX ships single-row strips (dirs:1); the loader replicates the row to
// all 8 dirs (B0-endorsed stopgap — identical rows, never engine rotate,
// which shimmers under nearest filtering). Full 8-dir sheets pass through.
namespace bh {

inline constexpr int kAtlasDirsFull = 8;

inline bool atlasDirsSupported(int dirs) { return dirs == kAtlasDirsFull || dirs == 1; }

// Source row for a facing dir: the only row for strips, identity for sheets.
inline int atlasRowForDir(int dirs, int dir) { return dirs == 1 ? 0 : dir; }

// Texture height a sheet/strip of frameH rows needs (bounds check).
inline int atlasRowsNeeded(int dirs) { return dirs == 1 ? 1 : kAtlasDirsFull; }

}  // namespace bh
