#include "assets/atlas.h"

#include <fstream>

#include <nlohmann/json.hpp>

namespace bh {

bool loadAtlas(const std::string& pngPath, const std::string& animJsonPath, Atlas& out) {
  out = Atlas{};
  try {
    if (!FileExists(pngPath.c_str()) || !FileExists(animJsonPath.c_str())) return false;
    Texture2D tex = LoadTexture(pngPath.c_str());
    if (tex.id == 0) return false;

    std::ifstream f(animJsonPath);
    nlohmann::json j;
    f >> j;

    const auto& anims = j.at("anims");
    for (auto it = anims.begin(); it != anims.end(); ++it) {
      const nlohmann::json& a = it.value();
      const int fw = a.at("frameW").get<int>();
      const int fh = a.at("frameH").get<int>();
      const int frames = a.at("frames").get<int>();
      const int dirs = a.value("dirs", 8);
      const int ox = a.value("offsetX", 0);
      const int oy = a.value("offsetY", 0);
      if (dirs != 8 || frames <= 0 || fw <= 0 || fh <= 0) return false;
      if (ox + frames * fw > tex.width || oy + 8 * fh > tex.height) return false;

      Anim an;
      an.fps = a.value("fps", 8.0f);
      for (int row = 0; row < 8; ++row) {
        for (int fr = 0; fr < frames; ++fr) {
          an.dirFrames[row].push_back(Rectangle{
              static_cast<float>(ox + fr * fw), static_cast<float>(oy + row * fh),
              static_cast<float>(fw), static_cast<float>(fh)});
        }
      }
      out.anims.emplace(it.key(), std::move(an));
    }
    out.tex = tex;
    out.ok = true;
    return true;
  } catch (...) {
    if (out.tex.id != 0) UnloadTexture(out.tex);
    out = Atlas{};
    return false;
  }
}

void unloadAtlas(Atlas& a) {
  if (a.tex.id != 0) UnloadTexture(a.tex);
  a = Atlas{};
}

Rectangle animFrame(const Atlas& a, const std::string& anim, int dir, double t) {
  const auto it = a.anims.find(anim);
  if (it == a.anims.end()) return Rectangle{0, 0, 0, 0};
  const Anim& an = it->second;
  const int d = (dir >= 0 && dir < 8) ? dir : 2;
  const std::vector<Rectangle>& v = an.dirFrames[d];
  if (v.empty()) return Rectangle{0, 0, 0, 0};
  int f = static_cast<int>(t * static_cast<double>(an.fps));
  if (f < 0) f = 0;
  return v[static_cast<size_t>(f) % v.size()];
}

}  // namespace bh
