// T-ART-12 textured ground: zone bake + skinned prisms (client-side).
#include "terrain_skin.h"

#include <cmath>
#include <cstdio>
#include <fstream>

#include <nlohmann/json.hpp>
#include <rlgl.h>

#include "render/iso.h"

namespace bh {
namespace {

constexpr int kTilePxW = 64;
constexpr int kTilePxH = 32;

int groundAt(const sim::Map& m, int x, int y, int fallback) {
  if (x < 0 || y < 0 || x >= m.w || y >= m.h) return fallback;
  return static_cast<int>(m.ground[static_cast<size_t>(y) * static_cast<size_t>(m.w) +
                                   static_cast<size_t>(x)]);
}

const char* pieceFile(int piece) {
  switch (piece) {
    case kTerrainPieceEdgeNE: return "edge_NE.png";
    case kTerrainPieceEdgeSE: return "edge_SE.png";
    case kTerrainPieceEdgeSW: return "edge_SW.png";
    case kTerrainPieceEdgeNW: return "edge_NW.png";
    case kTerrainPieceCornerN: return "corner_N.png";
    case kTerrainPieceCornerE: return "corner_E.png";
    case kTerrainPieceCornerS: return "corner_S.png";
    case kTerrainPieceCornerW: return "corner_W.png";
    case kTerrainPieceSkirtEdgeNE: return "skirt_edge_NE.png";
    case kTerrainPieceSkirtEdgeSE: return "skirt_edge_SE.png";
    case kTerrainPieceSkirtEdgeSW: return "skirt_edge_SW.png";
    case kTerrainPieceSkirtEdgeNW: return "skirt_edge_NW.png";
    case kTerrainPieceSkirtCornerN: return "skirt_corner_N.png";
    case kTerrainPieceSkirtCornerE: return "skirt_corner_E.png";
    case kTerrainPieceSkirtCornerS: return "skirt_corner_S.png";
    case kTerrainPieceSkirtCornerW: return "skirt_corner_W.png";
    default: return nullptr;
  }
}

bool isSkirt(int piece) { return piece >= kTerrainPieceSkirtEdgeNE; }
bool isCorner(int piece) {
  return (piece >= kTerrainPieceCornerN && piece <= kTerrainPieceCornerW) ||
         piece >= kTerrainPieceSkirtCornerN;
}

void faceQuad(Texture2D tex, Vector2 a, Vector2 ua, Vector2 b, Vector2 ub, Vector2 c,
              Vector2 uc, Vector2 d, Vector2 ud) {
  // Textured quad via rlgl. This raylib build draws shapes as RL_QUADS
  // (SUPPORT_QUADS_DRAW_MODE — see rshapes DrawTriangle), so faces go out
  // as quads too; vertex order matches iso::drawPrism's left/right tris.
  // UVs are top-down like DrawTexturePro source rects (verified in shots).
  rlSetTexture(tex.id);
  rlBegin(RL_QUADS);
  rlColor4ub(255, 255, 255, 255);
  rlTexCoord2f(ua.x / static_cast<float>(tex.width), ua.y / static_cast<float>(tex.height));
  rlVertex2f(a.x, a.y);
  rlTexCoord2f(ub.x / static_cast<float>(tex.width), ub.y / static_cast<float>(tex.height));
  rlVertex2f(b.x, b.y);
  rlTexCoord2f(uc.x / static_cast<float>(tex.width), uc.y / static_cast<float>(tex.height));
  rlVertex2f(c.x, c.y);
  rlTexCoord2f(ud.x / static_cast<float>(tex.width), ud.y / static_cast<float>(tex.height));
  rlVertex2f(d.x, d.y);
  rlEnd();
  rlSetTexture(0);
}

}  // namespace

const char* TerrainSkin::manifestDirFor(std::uint16_t mapId) {
  switch (mapId) {
    case 1: return "assets/aigen/terrain/town";
    case 2: return "assets/aigen/terrain/fields";
    case 3: return "assets/aigen/terrain/crypt/thornwall";
    case 4: return "assets/aigen/terrain/mine";
    case 5: return "assets/aigen/terrain/crypt/drowned";
    default: return "";  // castle + unknown: flat path (no terrain.json ships)
  }
}

void TerrainSkin::ensureFor(std::uint16_t mapId, const sim::Map& map) {
  if (baked_ && bakedFor_ == mapId) return;
  if (attempted_ && bakedFor_ == mapId) return;  // failed zone: stay flat
  unload();
  bakedFor_ = mapId;
  attempted_ = true;
  baked_ = bake(mapId, map);
}

void TerrainSkin::unload() {
  if (layer_.id != 0) UnloadTexture(layer_);
  layer_ = Texture2D{};
  for (int i = 0; i < 3; ++i) {
    if (top_[i].id != 0) UnloadTexture(top_[i]);
    if (left_[i].id != 0) UnloadTexture(left_[i]);
    if (right_[i].id != 0) UnloadTexture(right_[i]);
    top_[i] = Texture2D{};
    left_[i] = Texture2D{};
    right_[i] = Texture2D{};
  }
  prismSets_ = 0;
  for (auto& kv : pieceCache_) UnloadImage(kv.second);
  pieceCache_.clear();
  baked_ = false;
}

Image TerrainSkin::pieceImage(const std::string& path) {
  auto it = pieceCache_.find(path);
  if (it != pieceCache_.end()) return it->second;
  Image img{};
  if (FileExists(path.c_str())) img = LoadImage(path.c_str());
  pieceCache_.emplace(path, img);
  return img;
}

bool TerrainSkin::bake(std::uint16_t mapId, const sim::Map& map) {
  const char* mdir = manifestDirFor(mapId);
  if (mdir[0] == '\0' || map.w <= 0 || map.h <= 0) return false;
  const std::string manifest = std::string(mdir) + "/terrain.json";
  if (!FileExists(manifest.c_str())) return false;

  // ---- manifest: id names, plates, pairs ----
  std::unordered_map<std::string, int> nameToId;
  std::unordered_map<int, std::string> plateFile;
  std::vector<SkinPair> pairs;
  try {
    std::ifstream f(manifest);
    nlohmann::json j;
    f >> j;
    for (auto it = j.at("terrain_ids").begin(); it != j.at("terrain_ids").end(); ++it)
      nameToId[it.value().get<std::string>()] = std::atoi(it.key().c_str());
    for (auto it = j.at("plates").begin(); it != j.at("plates").end(); ++it)
      plateFile[nameToId[it.key()]] = it.value().at("file").get<std::string>();
    for (const auto& p : j.at("pairs")) {
      SkinPair sp;
      sp.law.a = nameToId[p.at("a").get<std::string>()];
      sp.law.b = nameToId[p.at("b").get<std::string>()];
      sp.law.bleeder = p.at("bleeder").is_null()
                           ? -1
                           : nameToId[p.at("bleeder").get<std::string>()];
      sp.law.variants = p.value("variants", 1);
      if (p.contains("dir")) sp.dir = p.at("dir").get<std::string>();
      pairs.push_back(std::move(sp));
    }
  } catch (...) {
    return false;
  }
  std::vector<TerrainPair> law;
  law.reserve(pairs.size());
  for (const auto& sp : pairs) law.push_back(sp.law);

  // ---- plates: load + 2x2 tile so cuts never wrap mid-crop ----
  struct TiledPlate {
    Image img{};
    bool ok = false;
  };
  std::unordered_map<int, TiledPlate> plates;
  int platesOk = 0;
  for (const auto& kv : plateFile) {
    TiledPlate tp;
    if (FileExists(kv.second.c_str())) {
      Image raw = LoadImage(kv.second.c_str());
      if (raw.data != nullptr && raw.width == 512 && raw.height == 256) {
        tp.img = GenImageColor(1024, 512, Color{0, 0, 0, 0});
        const Rectangle src{0, 0, 512, 256};
        for (int i = 0; i < 2; ++i)
          for (int j = 0; j < 2; ++j)
            ImageDraw(&tp.img, raw, src,
                      Rectangle{static_cast<float>(i * 512), static_cast<float>(j * 256),
                                512, 256},
                      WHITE);
        tp.ok = true;
        ++platesOk;
        UnloadImage(raw);
      } else if (raw.data != nullptr) {
        UnloadImage(raw);
      }
    }
    plates.emplace(kv.first, std::move(tp));
  }
  if (platesOk == 0) return false;

  // ---- diamond mask (white diamond on black, 64x32) ----
  Image mask = GenImageColor(kTilePxW, kTilePxH, Color{0, 0, 0, 255});
  for (int y = 0; y < kTilePxH; ++y)
    for (int x = 0; x < kTilePxW; ++x) {
      const float u = std::fabs(static_cast<float>(x) - 31.5f) / 32.0f;
      const float v = std::fabs(static_cast<float>(y) - 15.5f) / 16.0f;
      if (u + v <= 1.0f) ImageDrawPixel(&mask, x, y, Color{255, 255, 255, 255});
    }

  // ---- bake bounds from tile centers ----
  float minCx = 1e9f, maxCx = -1e9f, minCy = 1e9f, maxCy = -1e9f;
  for (int y = 0; y < map.h; ++y)
    for (int x = 0; x < map.w; ++x) {
      const Vector2 c = iso::tileToWorld(x, y, map.tileW, map.tileH);
      if (c.x < minCx) minCx = c.x;
      if (c.x > maxCx) maxCx = c.x;
      if (c.y < minCy) minCy = c.y;
      if (c.y > maxCy) maxCy = c.y;
    }
  const int bw = static_cast<int>(maxCx - minCx) + kTilePxW;
  const int bh = static_cast<int>(maxCy - minCy) + kTilePxH;
  layerOrigin_ = Vector2{minCx - kTilePxW / 2.0f, minCy - kTilePxH / 2.0f};
  Image layer = GenImageColor(bw, bh, Color{0, 0, 0, 0});

  int textured = 0, edges = 0;
  for (int y = 0; y < map.h; ++y) {
    for (int x = 0; x < map.w; ++x) {
      const int id = groundAt(map, x, y, -1);
      if (id == kTerrainWallId) continue;  // prisms stay dynamic (y-sort)
      const auto pit = plates.find(id);
      if (pit == plates.end() || !pit->second.ok) continue;  // flat underlay shows
      const Vector2 c = iso::tileToWorld(x, y, map.tileW, map.tileH);
      int sx = 0, sy = 0;
      terrainPlateCut(static_cast<int>(std::floor(c.x)), static_cast<int>(std::floor(c.y)),
                      512, 256, kTilePxW, kTilePxH, &sx, &sy);
      Image tile =
          ImageFromImage(pit->second.img, Rectangle{static_cast<float>(sx),
                                                    static_cast<float>(sy),
                                                    static_cast<float>(kTilePxW),
                                                    static_cast<float>(kTilePxH)});
      ImageAlphaMask(&tile, mask);
      const Rectangle dst{std::floor(c.x) - 32.0f - layerOrigin_.x,
                          std::floor(c.y) - 16.0f - layerOrigin_.y, 64, 32};
      ImageDraw(&layer, tile, Rectangle{0, 0, 64, 32}, dst, WHITE);
      UnloadImage(tile);
      ++textured;

      // D12 overlays: faces first, then points (suppressed by faces).
      int facePiece[4] = {kTerrainPieceNone, kTerrainPieceNone, kTerrainPieceNone,
                          kTerrainPieceNone};
      for (int f = 0; f < 4; ++f) {
        const int nx = x + kTerrainFaceDx[f], ny = y + kTerrainFaceDy[f];
        int variant = 0;
        facePiece[f] = terrainFacePiece(id, groundAt(map, nx, ny, id), f, x, y,
                                        law.data(), static_cast<int>(law.size()), &variant);
        if (facePiece[f] == kTerrainPieceNone) continue;
        const char* file = pieceFile(facePiece[f]);
        if (file == nullptr) {
          facePiece[f] = kTerrainPieceNone;
          continue;
        }
        std::string path;
        if (isSkirt(facePiece[f])) {
          path = std::string(mdir) + "/prism/" + file;
        } else {
          const int pi = terrainFindPair(id, groundAt(map, nx, ny, id), law.data(),
                                         static_cast<int>(law.size()));
          if (pi < 0 || pairs[static_cast<size_t>(pi)].dir.empty()) {
            facePiece[f] = kTerrainPieceNone;
            continue;
          }
          path = pairs[static_cast<size_t>(pi)].dir + "/v" + std::to_string(variant) +
                 "/" + file;
        }
        Image ov = pieceImage(path);
        if (ov.data == nullptr) {
          facePiece[f] = kTerrainPieceNone;
          continue;
        }
        ImageDraw(&layer, ov, Rectangle{0, 0, 64, 32}, dst, WHITE);
        ++edges;
      }
      for (int p = 0; p < 4; ++p) {
        const int nx = x + kTerrainPointDx[p], ny = y + kTerrainPointDy[p];
        const int fa = facePiece[kTerrainPointFaceA[p]] != kTerrainPieceNone ? 1 : 0;
        const int fb = facePiece[kTerrainPointFaceB[p]] != kTerrainPieceNone ? 1 : 0;
        int variant = 0;
        const int piece = terrainPointPiece(id, groundAt(map, nx, ny, id), p, fa, fb, x,
                                            y, law.data(),
                                            static_cast<int>(law.size()), &variant);
        if (piece == kTerrainPieceNone) continue;
        const char* file = pieceFile(piece);
        if (file == nullptr || !isCorner(piece)) continue;
        std::string path;
        if (isSkirt(piece)) {
          path = std::string(mdir) + "/prism/" + file;
        } else {
          const int pi = terrainFindPair(id, groundAt(map, nx, ny, id), law.data(),
                                         static_cast<int>(law.size()));
          if (pi < 0 || pairs[static_cast<size_t>(pi)].dir.empty()) continue;
          path = pairs[static_cast<size_t>(pi)].dir + "/v" + std::to_string(variant) +
                 "/" + file;
        }
        Image ov = pieceImage(path);
        if (ov.data == nullptr) continue;
        ImageDraw(&layer, ov, Rectangle{0, 0, 64, 32}, dst, WHITE);
        ++edges;
      }
    }
  }

  UnloadImage(mask);
  for (auto& kv : plates)
    if (kv.second.ok) UnloadImage(kv.second.img);
  layer_ = LoadTextureFromImage(layer);
  UnloadImage(layer);
  if (layer_.id == 0) return false;
  SetTextureFilter(layer_, TEXTURE_FILTER_POINT);

  // ---- prism skins (dynamic; cached textures). Single set
  // (prism/top.png, town/fields) or v0..v2 variant sets (mine/crypts —
  // the spec's anti-strobe rule for tunnel walls).
  const std::string prism = std::string(mdir) + "/prism/";
  auto loadSet = [&](const std::string& dir, int slot) {
    if (slot < 0 || slot > 2) return;
    Image t{};
    if (FileExists((dir + "top.png").c_str())) t = LoadImage((dir + "top.png").c_str());
    Image l{}, r{};
    if (FileExists((dir + "left.png").c_str())) l = LoadImage((dir + "left.png").c_str());
    if (FileExists((dir + "right.png").c_str())) r = LoadImage((dir + "right.png").c_str());
    if (t.data != nullptr && l.data != nullptr && r.data != nullptr && l.width == 32 &&
        l.height == 28 && r.width == 32 && r.height == 28) {
      top_[slot] = LoadTextureFromImage(t);
      left_[slot] = LoadTextureFromImage(l);
      right_[slot] = LoadTextureFromImage(r);
      SetTextureFilter(top_[slot], TEXTURE_FILTER_POINT);
      SetTextureFilter(left_[slot], TEXTURE_FILTER_POINT);
      SetTextureFilter(right_[slot], TEXTURE_FILTER_POINT);
      if (slot + 1 > prismSets_) prismSets_ = slot + 1;
    }
    if (t.data != nullptr) UnloadImage(t);
    if (l.data != nullptr) UnloadImage(l);
    if (r.data != nullptr) UnloadImage(r);
  };
  loadSet(prism, 0);
  for (int v = 0; v < 3; ++v) loadSet(prism + "v" + std::to_string(v) + "/", v);

  std::fprintf(stderr, "[terrain] zone %u baked: %d textured tiles, %d edge overlays%s\n",
               static_cast<unsigned>(mapId), textured, edges,
               prismReady() ? ", skinned prisms" : ", flat prisms (no skin)");
  return true;
}

void TerrainSkin::drawLayer() const {
  if (!baked_ || layer_.id == 0 || flatForced_) return;
  DrawTextureV(layer_, layerOrigin_, WHITE);
}

void TerrainSkin::drawPrism(float cx, float cy, int tileW, int tileH, int heightPx,
                            int variant) const {
  if (!prismReady() || flatForced_) return;
  const int set = prismSets_ > 0 ? (variant % prismSets_ + prismSets_) % prismSets_ : 0;
  const Texture2D top = top_[set], left = left_[set], right = right_[set];
  if (top.id == 0 || left.id == 0 || right.id == 0) return;
  const float h = static_cast<float>(heightPx);
  // top diamond (transparent corners in top.png)
  DrawTextureV(top, Vector2{cx - 32.0f, cy - h - 16.0f}, WHITE);
  // faces: quads from iso::drawPrism, textured with the 32x28 skins.
  // (tileW/H are 64x32 on every shipped map; the top art is 64x32 fixed.)
  const float hw = tileW * 0.5f;
  const float hh = tileH * 0.5f;
  const Vector2 tL{cx - hw, cy - h}, bL{cx - hw, cy}, bB{cx, cy + hh},
      tB{cx, cy + hh - h};
  const Vector2 tR{cx + hw, cy - h}, bR{cx + hw, cy};
  // left face (tL,bL,bB,tB), UVs top-down over the 32x28 skin
  faceQuad(left, tL, Vector2{0, 0}, bL, Vector2{0, 28}, bB, Vector2{32, 28}, tB,
           Vector2{32, 0});
  // right face (tB,bB,bR,tR)
  faceQuad(right, tB, Vector2{0, 0}, bB, Vector2{0, 28}, bR, Vector2{32, 28}, tR,
           Vector2{32, 0});
}

}  // namespace bh
