#pragma once

#include <raylib.h>

#include "sim/grid.h"

namespace bh::iso {

// 2:1 diamond projection. World units are pixels; (tx,ty) is a tile index
// (ints) or a tile-space float position (Vector2 variants).

Vector2 tileToWorld(int tx, int ty, int tileW, int tileH);
Vector2 tileToWorld(sim::TilePos t, int tileW, int tileH);
Vector2 tileToWorldF(Vector2 tileFrac, int tileW, int tileH);
// Inverse transform; worldToTilePos rounds to the nearest tile center,
// which is the right feel for click-to-move picking on diamonds.
Vector2 worldToTile(Vector2 world, int tileW, int tileH);
sim::TilePos worldToTilePos(Vector2 world, int tileW, int tileH);

void drawDiamond(Vector2 center, int tileW, int tileH, Color fill, Color edge);
void drawPrism(Vector2 center, int tileW, int tileH, int heightPx, Color top, Color left,
               Color right);

}  // namespace bh::iso
