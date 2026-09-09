#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "sim/astar.h"
#include "sim/grid.h"

namespace bh::sim {

// .bhmap — BLOODHOLLOW binary map format v2 (ADR-007).
// Header (little-endian): magic, version, w, h, tileW, tileH, spawnCount,
// portalCount, FNV-1a-64 checksum of payload.
// Payload: RLE u16 ground layer, RLE u16 zone layer, blocked bitfield,
// SpawnDef[], PortalDef[].
inline constexpr std::uint32_t kBhmapMagic = 0x504d4842u;  // "BHMP"
inline constexpr std::uint32_t kBhmapVersion = 2;  // v2: SpawnDef nightOnly (T-071)
inline constexpr int kBhmapMaxDim = 4096;

struct SpawnDef {
  std::int32_t x = 0, y = 0, w = 1, h = 1;  // tile-space rect
  std::uint32_t mobId = 0;                  // into shared/data/monsters.json (later)
  std::uint32_t maxAlive = 3;
  std::uint32_t respawnTicks = 1200;        // 60 s @20 Hz
  std::uint8_t nightOnly = 0;               // T-071: refill + aggro only at night
};

struct PortalDef {
  std::int32_t x = 0, y = 0, w = 1, h = 1;  // tile-space rect
  std::int32_t targetX = 0, targetY = 0;    // arrival tile in target map
  std::uint32_t targetMapId = 0;            // into shared/data/maps.json (later)
};

struct Map {
  std::int32_t w = 0, h = 0;
  std::int32_t tileW = 64, tileH = 32;
  std::vector<std::uint16_t> ground;   // h*w, terrain type id
  std::vector<std::uint16_t> zone;     // h*w, 0 = none
  std::vector<std::uint8_t> blocked;   // h*w, nonzero = blocked
  std::vector<SpawnDef> spawners;
  std::vector<PortalDef> portals;

  bool isBlocked(int x, int y) const {
    if (x < 0 || y < 0 || x >= w || y >= h) return true;
    return blocked[static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x)] != 0;
  }
  CostGrid costGrid() const { return CostGrid{w, h, blocked.data()}; }
  bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
};

std::uint64_t fnv1a64(const std::uint8_t* data, size_t n,
                      std::uint64_t seed = 1469598103934665603ULL);

bool saveBhmap(const std::string& path, const Map& m, std::string* err);
std::optional<Map> loadBhmap(const std::string& path, std::string* err);

}  // namespace bh::sim
