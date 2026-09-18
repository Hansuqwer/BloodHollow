// T-ART-12 textured-ground law pins (raylib-free header, runs headless).
// D6: plate cut by world coords, prism skins, D12 adjacency edge lookup.
// D12: organic-over-built bleed; WALL never bleeds (footing skirt instead);
// points draw only when neither adjoining face bleeds.
#include <doctest/doctest.h>

#include "render/terrain_skin.h"
#include "assets/atlas_dirs.h"

using namespace bh;

TEST_CASE("T-ART-12: plate cut wraps into the plate, center-stable") {
  int sx = -1, sy = -1;
  terrainPlateCut(100, 60, 512, 256, 64, 32, &sx, &sy);
  CHECK(sx == 68);
  CHECK(sy == 44);
  // negative centers wrap, never negative out
  terrainPlateCut(-1504, -100, 512, 256, 64, 32, &sx, &sy);
  CHECK(sx >= 0);
  CHECK(sx < 512);
  CHECK(sy >= 0);
  CHECK(sy < 256);
  // same tile -> same cut (deterministic, no RNG stream)
  int ax = 0, ay = 0, bx = 0, by = 0;
  terrainPlateCut(812, 404, 512, 256, 64, 32, &ax, &ay);
  terrainPlateCut(812, 404, 512, 256, 64, 32, &bx, &by);
  CHECK(ax == bx);
  CHECK(ay == by);
}

TEST_CASE("T-ART-12: variant hash is deterministic and bounded") {
  for (int x = -40; x < 40; ++x)
    for (int y = -40; y < 40; ++y) {
      const int v = terrainVariant(x, y, 3);
      CHECK(v >= 0);
      CHECK(v < 3);
      CHECK(terrainVariant(x, y, 3) == v);
    }
  CHECK(terrainVariant(0, 0, 1) == 0);
  CHECK(terrainVariant(5, 7, 0) == 0);
}

TEST_CASE("T-ART-12: D12 face bleed — overlay draws on the base tile only") {
  // GRASS=0 bleeds onto PATH=5 (PATH_GRASS pair: base PATH, overlay GRASS).
  // The overlay draws on the BASE tile facing the bleeder — never on the
  // bleeder itself.
  const TerrainPair pairs[] = {{0, 5, 0, 3}, {0, 2, -1, 1}};  // + GRASS/WALL skirt
  int v = -1;
  CHECK(terrainFacePiece(5, 0, kTerrainFaceNE, 3, 4, pairs, 2, &v) == kTerrainPieceEdgeNE);
  CHECK(v >= 0);
  CHECK(v < 3);
  CHECK(terrainFacePiece(0, 5, kTerrainFaceNE, 3, 4, pairs, 2, &v) == kTerrainPieceNone);
  // same material: nothing
  CHECK(terrainFacePiece(0, 0, kTerrainFaceSE, 3, 4, pairs, 2, &v) == kTerrainPieceNone);
  // unknown adjacency: nothing (flat plate, no guess)
  CHECK(terrainFacePiece(0, 7, kTerrainFaceSW, 3, 4, pairs, 2, &v) == kTerrainPieceNone);
  // bad face: nothing
  CHECK(terrainFacePiece(5, 0, 9, 3, 4, pairs, 2, &v) == kTerrainPieceNone);
}

TEST_CASE("T-ART-12: WALL never bleeds — footing skirt instead") {
  const TerrainPair pairs[] = {{0, 2, -1, 1}};
  int v = -1;
  CHECK(terrainFacePiece(0, 2, kTerrainFaceSE, 10, 11, pairs, 1, &v) ==
        kTerrainPieceSkirtEdgeSE);
  CHECK(v == 0);
  // the WALL tile itself draws no edge overlay (prism skin covers it)
  CHECK(terrainFacePiece(2, 0, kTerrainFaceNW, 10, 11, pairs, 1, &v) ==
        kTerrainPieceSkirtEdgeNW);
}

TEST_CASE("T-ART-12: points draw only when neither adjoining face bleeds") {
  const TerrainPair pairs[] = {{0, 5, 0, 3}};
  int v = -1;
  // PATH tile, diagonal GRASS, faces quiet -> corner piece (N(adjoin NW,NE))
  CHECK(terrainPointPiece(5, 0, kTerrainPointN, 0, 0, 3, 4, pairs, 1, &v) ==
        kTerrainPieceCornerN);
  // either adjoining face bleeding suppresses the point
  CHECK(terrainPointPiece(5, 0, kTerrainPointN, 1, 0, 3, 4, pairs, 1, &v) ==
        kTerrainPieceNone);
  CHECK(terrainPointPiece(5, 0, kTerrainPointN, 0, 1, 3, 4, pairs, 1, &v) ==
        kTerrainPieceNone);
  // same material diagonal: nothing
  CHECK(terrainPointPiece(5, 5, kTerrainPointE, 0, 0, 3, 4, pairs, 1, &v) ==
        kTerrainPieceNone);
}

TEST_CASE("T-ART-12: adjacency offsets match iso projection (+tx=SE, +ty=SW)") {
  // faces
  CHECK(kTerrainFaceDx[kTerrainFaceNE] == 0);
  CHECK(kTerrainFaceDy[kTerrainFaceNE] == -1);
  CHECK(kTerrainFaceDx[kTerrainFaceSE] == 1);
  CHECK(kTerrainFaceDy[kTerrainFaceSE] == 0);
  CHECK(kTerrainFaceDx[kTerrainFaceSW] == 0);
  CHECK(kTerrainFaceDy[kTerrainFaceSW] == 1);
  CHECK(kTerrainFaceDx[kTerrainFaceNW] == -1);
  CHECK(kTerrainFaceDy[kTerrainFaceNW] == 0);
  // points
  CHECK(kTerrainPointDx[kTerrainPointN] == -1);
  CHECK(kTerrainPointDy[kTerrainPointN] == -1);
  CHECK(kTerrainPointDx[kTerrainPointE] == 1);
  CHECK(kTerrainPointDy[kTerrainPointE] == -1);
  // point/face adjacency: N sits between NW and NE
  CHECK(kTerrainPointFaceA[kTerrainPointN] == kTerrainFaceNW);
  CHECK(kTerrainPointFaceB[kTerrainPointN] == kTerrainFaceNE);
  CHECK(kTerrainPointFaceA[kTerrainPointE] == kTerrainFaceNE);
  CHECK(kTerrainPointFaceB[kTerrainPointE] == kTerrainFaceSE);
}

TEST_CASE("T-ART-14: dirs:1 strips replicate the row, 8-dir sheets pass through") {
  CHECK(atlasDirsSupported(8));
  CHECK(atlasDirsSupported(1));
  CHECK_FALSE(atlasDirsSupported(0));
  CHECK_FALSE(atlasDirsSupported(2));
  CHECK_FALSE(atlasDirsSupported(4));
  for (int d = 0; d < 8; ++d) {
    CHECK(atlasRowForDir(8, d) == d);  // sheets untouched
    CHECK(atlasRowForDir(1, d) == 0);  // strips: every facing reads row 0
  }
  CHECK(atlasRowsNeeded(8) == 8);
  CHECK(atlasRowsNeeded(1) == 1);  // bounds check shrinks with the strip
}
