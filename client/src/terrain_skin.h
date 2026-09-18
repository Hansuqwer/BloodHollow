#pragma once

// T-ART-12 textured ground (client-side). Bakes the zone's ground diamonds +
// D12 edge overlays into one static texture at zone load (plates cut by world
// coords, paint-then-cut), and draws skinned prisms for WALL tiles per frame
// (y-sorted with entities, so they stay dynamic). Any missing art degrades
// to the legacy flat path per tile — never holes. Render-only: no sim, no
// wire, no epoch.
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <raylib.h>

#include "render/terrain_skin.h"
#include "sim/bhmap.h"

namespace bh {

class TerrainSkin {
 public:
  ~TerrainSkin() { unload(); }

  // No-op when already baked for mapId. Safe to call every frame.
  void ensureFor(std::uint16_t mapId, const sim::Map& map);
  void unload();

  bool ready() const { return baked_; }       // ground layer available
  bool prismReady() const { return prismSets_ > 0; }
  void setFlatForced(bool f) { flatForced_ = f; }  // --flat-ground dev shots
  bool flatForced() const { return flatForced_; }

  void drawLayer() const;  // baked diamonds+edges (call after the flat pass)
  // Skinned prism at tile-center (cx,cy). variant picks the prism set
  // (mine/crypt ship v0..v2 so tunnel walls don't strobe). Caller keeps the
  // legacy iso::drawPrism path when !prismReady().
  void drawPrism(float cx, float cy, int tileW, int tileH, int heightPx,
                 int variant) const;

 private:
  struct SkinPair {
    TerrainPair law{};
    std::string dir;  // edges/<dir>/v<variant>/<piece>.png
  };

  static const char* manifestDirFor(std::uint16_t mapId);
  bool bake(std::uint16_t mapId, const sim::Map& map);
  Image pieceImage(const std::string& path);  // cached, CPU-side

  std::uint16_t bakedFor_ = 0;
  bool baked_ = false;
  bool attempted_ = false;  // a failed zone stays on the flat path, no retry spam
  bool flatForced_ = false;
  Texture2D layer_{};
  Vector2 layerOrigin_{};
  Texture2D top_[3]{}, left_[3]{}, right_[3]{};  // prism sets (v0..v2)
  int prismSets_ = 0;
  std::unordered_map<std::string, Image> pieceCache_{};
};

}  // namespace bh
