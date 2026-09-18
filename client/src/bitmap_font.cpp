// T-ART-13 bitmap callout font: white-glyph strip + JSON metrics.
#include "bitmap_font.h"

#include <cctype>
#include <fstream>

#include <nlohmann/json.hpp>

namespace bh {

bool BitmapFont::load(const std::string& pngPath, const std::string& jsonPath) {
  unload();
  try {
    if (!FileExists(pngPath.c_str()) || !FileExists(jsonPath.c_str())) return false;
    Texture2D tex = LoadTexture(pngPath.c_str());
    if (tex.id == 0) return false;
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);  // era pixel look (T-ART-01)

    std::ifstream f(jsonPath);
    nlohmann::json j;
    f >> j;
    capHeight_ = j.value("size", 11);
    for (const auto& c : j.at("chars")) {
      const int id = c.at("id").get<int>();
      BitmapGlyph g;
      g.src = Rectangle{static_cast<float>(c.at("x").get<int>()),
                        static_cast<float>(c.at("y").get<int>()),
                        static_cast<float>(c.at("width").get<int>()),
                        static_cast<float>(c.at("height").get<int>())};
      g.advance = c.at("xadvance").get<int>();
      glyphs_.emplace(id, g);
    }
    auto sp = glyphs_.find(' ');
    spaceAdvance_ = sp != glyphs_.end() ? sp->second.advance : capHeight_ / 2;
    tex_ = tex;
    return true;
  } catch (...) {
    unload();
    return false;
  }
}

void BitmapFont::unload() {
  if (tex_.id != 0) UnloadTexture(tex_);
  tex_ = Texture2D{};
  glyphs_.clear();
}

int BitmapFont::measure(const std::string& text) const {
  int w = 0;
  for (unsigned char ch : text) {
    const int up = std::toupper(ch);
    auto it = glyphs_.find(up);
    w += it != glyphs_.end() ? it->second.advance : spaceAdvance_;
  }
  return w;
}

void BitmapFont::drawText(const std::string& text, int x, int y, Color color) const {
  if (tex_.id == 0) return;
  int cx = x;
  for (unsigned char ch : text) {
    const int up = std::toupper(ch);
    auto it = glyphs_.find(up);
    if (it == glyphs_.end()) {
      cx += spaceAdvance_;
      continue;
    }
    const Rectangle& s = it->second.src;
    // 1px black outline, then the tinted fill (same texture, one batch).
    const Color black{0, 0, 0, color.a};
    DrawTexturePro(tex_, s,
                   Rectangle{static_cast<float>(cx - 1), static_cast<float>(y), s.width,
                             s.height},
                   Vector2{0, 0}, 0.0f, black);
    DrawTexturePro(tex_, s,
                   Rectangle{static_cast<float>(cx + 1), static_cast<float>(y), s.width,
                             s.height},
                   Vector2{0, 0}, 0.0f, black);
    DrawTexturePro(tex_, s,
                   Rectangle{static_cast<float>(cx), static_cast<float>(y - 1), s.width,
                             s.height},
                   Vector2{0, 0}, 0.0f, black);
    DrawTexturePro(tex_, s,
                   Rectangle{static_cast<float>(cx), static_cast<float>(y + 1), s.width,
                             s.height},
                   Vector2{0, 0}, 0.0f, black);
    DrawTexturePro(tex_, s,
                   Rectangle{static_cast<float>(cx), static_cast<float>(y), s.width,
                             s.height},
                   Vector2{0, 0}, 0.0f, color);
    cx += it->second.advance;
  }
}

}  // namespace bh
