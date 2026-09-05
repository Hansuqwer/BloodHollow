#pragma once

#include <raylib.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace bh {

// Uniform-grid animation atlas: every anim is 8 direction rows x N frames,
// laid out on a regular grid. Format produced by tools/atlaspack (later)
// and by the procedural placeholder generator (T-004).
struct Anim {
  std::vector<Rectangle> dirFrames[8];  // indexed by sim dir index 0..7
  float fps = 8.0f;
};

struct Atlas {
  Texture2D tex{};
  std::unordered_map<std::string, Anim> anims;
  bool ok = false;
};

// anim JSON schema (v1):
// {"anims": {"walk": {"frameW":32,"frameH":48,"frames":6,"dirs":8,
//                     "fps":10,"offsetX":0,"offsetY":0}}}
bool loadAtlas(const std::string& pngPath, const std::string& animJsonPath, Atlas& out);
void unloadAtlas(Atlas& a);

// Frame rect for (anim, dir) at animation time t seconds. Empty rect on miss.
Rectangle animFrame(const Atlas& a, const std::string& anim, int dir, double t);

}  // namespace bh
