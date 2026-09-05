#include "assets/placeholder.h"

#include <cmath>

#include "sim/grid.h"

namespace bh {

namespace {
// Filled ellipse via scanlines (raylib 5.5 image API has no ImageDrawEllipse).
void drawEllipse(Image* img, int cx, int cy, int rx, int ry, Color c) {
  for (int y = -ry; y <= ry; ++y) {
    const float yn = static_cast<float>(y) / static_cast<float>(ry);
    const int half = static_cast<int>(std::sqrt(1.0f - yn * yn) * rx);
    ImageDrawLine(img, cx - half, cy + y, cx + half, cy + y, c);
  }
}
}  // namespace

Atlas makeHeroAtlas(Color body, Color trim, Color skin) {
  constexpr int kFw = 32;
  constexpr int kFh = 48;
  constexpr int kCols = 7;  // frames 0..5 = walk cycle, col 6 = idle
  constexpr int kRows = 8;  // directions 0..7 (sim::kDx/kDy order)

  Atlas out;
  Image img = GenImageColor(kFw * kCols, kFh * kRows, Color{0, 0, 0, 0});

  for (int dir = 0; dir < kRows; ++dir) {
    const int ddx = sim::kDx[dir];
    const int ddy = sim::kDy[dir];
    for (int f = 0; f < kCols; ++f) {
      const int ox = f * kFw;
      const int oy = dir * kFh;
      const int cx = ox + 16;
      const int feet = oy + 42;
      const bool walk = f < 6;
      const float ph = static_cast<float>(f);
      const int bob = walk ? static_cast<int>(std::sin(ph * 6.2831853f / 6.0f) * 1.5f) : 0;
      const int leg = walk ? ((f % 2 == 0) ? 1 : 0) : 0;

      drawEllipse(&img, cx, feet + 3, 9, 3, Color{0, 0, 0, 70});
      ImageDrawRectangle(&img, cx - 6, feet - 8 + (leg == 1 ? 1 : 0), 5, 8,
                         Color{40, 34, 34, 255});
      ImageDrawRectangle(&img, cx + 1, feet - 8 + (leg == 0 ? 1 : 0), 5, 8,
                         Color{40, 34, 34, 255});
      ImageDrawRectangle(&img, cx - 8, feet - 24 - bob, 16, 17, body);
      ImageDrawRectangle(&img, cx - 8, feet - 12 - bob, 16, 3, trim);
      ImageDrawCircle(&img, cx, feet - 27 - bob, 8, body);
      ImageDrawCircle(&img, cx + ddx * 3, feet - 27 - bob + ddy * 2, 4, skin);
    }
  }

  out.tex = LoadTextureFromImage(img);
  UnloadImage(img);
  if (out.tex.id == 0) return out;
  SetTextureFilter(out.tex, TEXTURE_FILTER_POINT);

  auto addAnim = [&](const char* name, int col0, int frames, float fps) {
    Anim an;
    an.fps = fps;
    for (int row = 0; row < kRows; ++row) {
      for (int fr = 0; fr < frames; ++fr) {
        an.dirFrames[row].push_back(Rectangle{
            static_cast<float>((col0 + fr) * kFw), static_cast<float>(row * kFh),
            static_cast<float>(kFw), static_cast<float>(kFh)});
      }
    }
    out.anims.emplace(name, std::move(an));
  };
  addAnim("walk", 0, 6, 10.0f);
  addAnim("idle", 6, 1, 1.0f);
  out.ok = true;
  return out;
}

}  // namespace bh
