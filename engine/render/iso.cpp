#include "render/iso.h"

#include <cmath>

namespace bh::iso {

Vector2 tileToWorld(int tx, int ty, int tileW, int tileH) {
  return Vector2{static_cast<float>(tx - ty) * (tileW * 0.5f),
                 static_cast<float>(tx + ty) * (tileH * 0.5f)};
}

Vector2 tileToWorld(sim::TilePos t, int tileW, int tileH) {
  return tileToWorld(static_cast<int>(t.x), static_cast<int>(t.y), tileW, tileH);
}

Vector2 tileToWorldF(Vector2 tileFrac, int tileW, int tileH) {
  return Vector2{(tileFrac.x - tileFrac.y) * (tileW * 0.5f),
                 (tileFrac.x + tileFrac.y) * (tileH * 0.5f)};
}

Vector2 worldToTile(Vector2 world, int tileW, int tileH) {
  const float hw = tileW * 0.5f;
  const float hh = tileH * 0.5f;
  return Vector2{(world.x / hw + world.y / hh) * 0.5f, (world.y / hh - world.x / hw) * 0.5f};
}

sim::TilePos worldToTilePos(Vector2 world, int tileW, int tileH) {
  const Vector2 t = worldToTile(world, tileW, tileH);
  return sim::TilePos{static_cast<int>(std::floor(t.x + 0.5f)),
                      static_cast<int>(std::floor(t.y + 0.5f))};
}

void drawDiamond(Vector2 c, int tileW, int tileH, Color fill, Color edge) {
  const Vector2 top{c.x, c.y - tileH * 0.5f};
  const Vector2 right{c.x + tileW * 0.5f, c.y};
  const Vector2 bottom{c.x, c.y + tileH * 0.5f};
  const Vector2 left{c.x - tileW * 0.5f, c.y};
  // raylib culls the other winding (verified empirically, see docs/devlog).
  // Screen-space CCW order of the diamond corners: top -> left -> bottom -> right.
  DrawTriangle(c, top, left, fill);
  DrawTriangle(c, left, bottom, fill);
  DrawTriangle(c, bottom, right, fill);
  DrawTriangle(c, right, top, fill);
  if (edge.a > 0) {
    DrawLineV(top, right, edge);
    DrawLineV(right, bottom, edge);
    DrawLineV(bottom, left, edge);
    DrawLineV(left, top, edge);
  }
}

void drawPrism(Vector2 c, int tileW, int tileH, int heightPx, Color topColor, Color leftColor,
               Color rightColor) {
  const float h = static_cast<float>(heightPx);
  const Vector2 cTop{c.x, c.y - h};
  // ground diamond corners
  const Vector2 bL{c.x - tileW * 0.5f, c.y};
  const Vector2 bB{c.x, c.y + tileH * 0.5f};
  const Vector2 bR{c.x + tileW * 0.5f, c.y};
  // raised corners
  const Vector2 tL{bL.x, bL.y - h};
  const Vector2 tB{bB.x, bB.y - h};
  const Vector2 tR{bR.x, bR.y - h};
  // side faces (quads as two triangles)
  DrawTriangle(tL, bL, bB, leftColor);
  DrawTriangle(tL, bB, tB, leftColor);
  DrawTriangle(tB, bB, bR, rightColor);
  DrawTriangle(tB, bR, tR, rightColor);
  // top face (screen-space CCW: top -> left -> bottom -> right)
  const Vector2 tT{tB.x, tB.y - tileH * 0.5f};
  DrawTriangle(cTop, tT, tL, topColor);
  DrawTriangle(cTop, tL, tB, topColor);
  DrawTriangle(cTop, tB, tR, topColor);
  DrawTriangle(cTop, tR, tT, topColor);
}

}  // namespace bh::iso
