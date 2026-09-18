#pragma once

// T-ART-13 bitmap callout font (client-side). bhfont.py exports a white-glyph
// strip + BMFont-like JSON (assets/aigen/ui/font/); the client tints per
// draw (callout-kind colours, karma name tints) with a 1px black outline.
// Missing asset -> !ok() -> callers keep the legacy DrawText path.
#include <string>
#include <unordered_map>

#include <raylib.h>

namespace bh {

struct BitmapGlyph {
  Rectangle src{};
  int advance = 0;
};

class BitmapFont {
 public:
  ~BitmapFont() { unload(); }
  bool load(const std::string& pngPath, const std::string& jsonPath);
  void unload();
  bool ok() const { return tex_.id != 0; }
  int capHeight() const { return capHeight_; }

  int measure(const std::string& text) const;
  // Uppercase red-caps read (the set is caps-only); unknown glyphs collapse
  // to a space advance. Outline always black; fill takes color incl. alpha.
  void drawText(const std::string& text, int x, int y, Color color) const;

 private:
  Texture2D tex_{};
  std::unordered_map<int, BitmapGlyph> glyphs_;
  int spaceAdvance_ = 4;
  int capHeight_ = 11;
};

}  // namespace bh
