#pragma once

// T-ART-12 textured-ground law (render-side, raylib-free so the unit suite
// can pin it — lightmask.h pattern). B0 rulings D6 (textured diamond draw,
// plate cut by world coords, prism skins, D12 adjacency edge lookup) + D12
// (organic-over-built bleed; WALL never bleeds — footing skirt instead).
//
// Art contract (assets/aigen/terrain/<zone>/terrain.json):
// - plates: one 512x256 painting per ground id; a tile's diamond is cut at
//   (tileCenterPx mod plate) — paint-then-cut, continuous by construction.
// - edges: per adjacency pair dir, v0..v2 variants, 8 pieces of 64x32
//   (edge_NE/NW/SE/SW + corner_N/E/S/W). Overlay drawn on the BASE tile.
// - prism: top 64x32 + left/right 32x28 faces + skirt_edge_*/skirt_corner_*
//   64x32 overlays for ground tiles touching a WALL.
//
// Tile-space adjacency (engine/render/iso.cpp tileToWorld: +tx moves screen
// SE, +ty moves screen SW):
//   faces: NE=(0,-1) SE=(+1,0) SW=(0,+1) NW=(-1,0)
//   points: N=(-1,-1) E=(+1,-1) S=(+1,+1) W=(-1,+1)
// A point piece draws only when neither adjoining face bleeds (D12).

namespace bh {

// Ground id reserved for WALL (prism) on every shipped map: standard
// 0 GRASS/1 DIRT/2 WALL/... and crypt 0 STONE/1 FLOOR/2 WALL/... agree
// (docs/art/00-VERIFY.md #20). Walls draw skinned prisms, never plates.
inline constexpr int kTerrainWallId = 2;

// Face ids (also index edgeOffsets).
inline constexpr int kTerrainFaceNE = 0;
inline constexpr int kTerrainFaceSE = 1;
inline constexpr int kTerrainFaceSW = 2;
inline constexpr int kTerrainFaceNW = 3;
// Point ids.
inline constexpr int kTerrainPointN = 0;
inline constexpr int kTerrainPointE = 1;
inline constexpr int kTerrainPointS = 2;
inline constexpr int kTerrainPointW = 3;

// (dx,dy) in tile space for faces and points.
inline constexpr int kTerrainFaceDx[4] = {0, 1, 0, -1};
inline constexpr int kTerrainFaceDy[4] = {-1, 0, 1, 0};
inline constexpr int kTerrainPointDx[4] = {-1, 1, 1, -1};
inline constexpr int kTerrainPointDy[4] = {-1, -1, 1, 1};
// Adjoining faces for each point: N:(NW,NE) E:(NE,SE) S:(SE,SW) W:(SW,NW).
inline constexpr int kTerrainPointFaceA[4] = {3, 0, 1, 2};
inline constexpr int kTerrainPointFaceB[4] = {0, 1, 2, 3};

// Piece ids. NONE = draw nothing (flat plate diamond is enough).
inline constexpr int kTerrainPieceNone = 0;
inline constexpr int kTerrainPieceEdgeNE = 1;
inline constexpr int kTerrainPieceEdgeSE = 2;
inline constexpr int kTerrainPieceEdgeSW = 3;
inline constexpr int kTerrainPieceEdgeNW = 4;
inline constexpr int kTerrainPieceCornerN = 5;
inline constexpr int kTerrainPieceCornerE = 6;
inline constexpr int kTerrainPieceCornerS = 7;
inline constexpr int kTerrainPieceCornerW = 8;
inline constexpr int kTerrainPieceSkirtEdgeNE = 9;
inline constexpr int kTerrainPieceSkirtEdgeSE = 10;
inline constexpr int kTerrainPieceSkirtEdgeSW = 11;
inline constexpr int kTerrainPieceSkirtEdgeNW = 12;
inline constexpr int kTerrainPieceSkirtCornerN = 13;
inline constexpr int kTerrainPieceSkirtCornerE = 14;
inline constexpr int kTerrainPieceSkirtCornerS = 15;
inline constexpr int kTerrainPieceSkirtCornerW = 16;

// One adjacency pair from terrain.json. bleeder < 0 marks a wall-type pair
// (WALL never bleeds — the ground tile draws a footing skirt instead).
// Otherwise bleeder is the overlay material id (== a or == b); the overlay
// draws only on tiles of the OTHER (base) material. variants = v0..vN dirs.
struct TerrainPair {
  int a = -1;
  int b = -1;
  int bleeder = -2;
  int variants = 1;
};

// Unordered pair match; -1 when the adjacency has no painted pair.
inline int terrainFindPair(int x, int y, const TerrainPair* pairs, int npairs) {
  if (pairs == nullptr || npairs <= 0) return -1;
  for (int i = 0; i < npairs; ++i) {
    if ((pairs[i].a == x && pairs[i].b == y) || (pairs[i].a == y && pairs[i].b == x))
      return i;
  }
  return -1;
}

// Deterministic per-tile variant in [0, variants). Render-only hash — never
// gameplay (no sim/rng.h here by design).
inline int terrainVariant(int tx, int ty, int variants) {
  if (variants <= 1) return 0;
  unsigned h = static_cast<unsigned>(tx) * 73856093u ^
               static_cast<unsigned>(ty) * 19349663u ^ 0x9e3779b9u;
  h ^= h >> 13;
  return static_cast<int>(h % static_cast<unsigned>(variants));
}

// Plate cut origin for a tile whose diamond center is at (tileCx, tileCy) px.
// The cut box (cutW x cutH) is anchored so the center pixel is stable;
// coords wrap into the plate.
inline void terrainPlateCut(int tileCx, int tileCy, int plateW, int plateH, int cutW,
                            int cutH, int* sx, int* sy) {
  int x = (tileCx - cutW / 2) % plateW;
  int y = (tileCy - cutH / 2) % plateH;
  if (x < 0) x += plateW;
  if (y < 0) y += plateH;
  if (sx) *sx = x;
  if (sy) *sy = y;
}

// Overlay piece for the tile face `face` (0..3) whose across-face neighbour
// has ground id `across` (`center` = this tile's id). variantOut takes the
// v-dir when an edge piece draws (0 for skirts / NONE).
inline int terrainFacePiece(int center, int across, int face, int tx, int ty,
                            const TerrainPair* pairs, int npairs, int* variantOut) {
  if (variantOut) *variantOut = 0;
  if (face < 0 || face > 3 || across == center) return kTerrainPieceNone;
  const int pi = terrainFindPair(center, across, pairs, npairs);
  if (pi < 0) return kTerrainPieceNone;
  const TerrainPair& p = pairs[pi];
  if (p.bleeder < 0) return kTerrainPieceSkirtEdgeNE + face;  // WALL footing
  if (p.bleeder != across) return kTerrainPieceNone;  // we ARE the bleeder
  if (variantOut) *variantOut = terrainVariant(tx, ty, p.variants);
  return kTerrainPieceEdgeNE + face;
}

// Overlay piece for the tile point `point` (0..3) whose diagonal neighbour
// has ground id `p`. faceABleeds/faceBBleeds = whether the two adjoining
// faces already draw an overlay (faces cover points — D12).
inline int terrainPointPiece(int center, int p, int point, int faceABleeds,
                             int faceBBleeds, int tx, int ty, const TerrainPair* pairs,
                             int npairs, int* variantOut) {
  if (variantOut) *variantOut = 0;
  if (point < 0 || point > 3 || p == center) return kTerrainPieceNone;
  if (faceABleeds || faceBBleeds) return kTerrainPieceNone;
  const int pi = terrainFindPair(center, p, pairs, npairs);
  if (pi < 0) return kTerrainPieceNone;
  const TerrainPair& pr = pairs[pi];
  if (pr.bleeder < 0) return kTerrainPieceSkirtCornerN + point;
  if (pr.bleeder != p) return kTerrainPieceNone;
  if (variantOut) *variantOut = terrainVariant(tx, ty, pr.variants);
  return kTerrainPieceCornerN + point;
}

}  // namespace bh
