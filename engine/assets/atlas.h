#pragma once

#include <raylib.h>

#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

#include "content/mobs.h"
#include "content/wirekind.h"

namespace bh {

// Uniform-grid animation atlas: every anim is 8 direction rows x N frames,
// laid out on a regular grid. Format produced by tools/atlaspack (B3/B4 mobs,
// B5 NPC sheets) and by the procedural placeholder generator (T-004 hero).
struct Anim {
  std::vector<Rectangle> dirFrames[8];  // indexed by sim dir index 0..7
  float fps = 8.0f;
  float anchorY = 42.0f;  // T-ART-10: feet offset (px from cell top)
};

struct Atlas {
  Texture2D tex{};
  std::unordered_map<std::string, Anim> anims;
  bool ok = false;
};

// anim JSON schema (v1 + T-ART-10 anchorY):
// {"anims": {"walk": {"frameW":32,"frameH":48,"frames":6,"dirs":8,
//                     "fps":10,"offsetX":0,"offsetY":0,"anchorY":42}}}
bool loadAtlas(const std::string& pngPath, const std::string& animJsonPath, Atlas& out);
void unloadAtlas(Atlas& a);

// Frame rect for (anim, dir) at animation time t seconds. Empty rect on miss.
Rectangle animFrame(const Atlas& a, const std::string& anim, int dir, double t);
// Feet anchor for an anim (42.0 legacy default when absent). Inline: the unit
// suite pins it without linking the engine (which needs a display to load).
inline float animAnchorY(const Atlas& a, const std::string& anim) {
  const auto it = a.anims.find(anim);
  if (it == a.anims.end()) return 42.0f;
  return it->second.anchorY;
}

// T-ART-05 sheet-dir law (headless-testable): mob kinds index kMobs and
// resolve to assets/aigen/mobs/<mobId>_<slug>/ (slug = lowercased name,
// spaces to underscores — matches every shipped B3/B4 row). Returns false
// for players (0), furniture (64+), and out-of-range kinds: those render
// the placeholder hero and never reach the loader.
inline bool mobSheetPaths(std::uint8_t kind, char* png, std::size_t pngN, char* js,
                          std::size_t jsN) {
  if (kind == 0 || kind >= content::kWireKindFurnitureFloor ||
      kind > content::kMobKindCount) {
    return false;
  }
  const content::MobDef& def = content::kMobs[kind - 1];
  std::string slug;
  for (const char c : std::string(def.name)) {
    if (c == ' ') slug.push_back('_');
    else if (c >= 'A' && c <= 'Z') slug.push_back(static_cast<char>(c + ('a' - 'A')));
    else slug.push_back(c);
  }
  std::snprintf(png, pngN, "assets/aigen/mobs/%u_%s/sheet.png", def.mobId, slug.c_str());
  std::snprintf(js, jsN, "assets/aigen/mobs/%u_%s/sheet.json", def.mobId, slug.c_str());
  return true;
}

// T-ART-B5: furniture / NPC sheet-dir law. Furniture kinds (64+) live under
// assets/aigen/npcs/<slug>/sheet.png+json. The slug mapping is explicit here
// (furniture don't have a kMobs-style table we can slugify from); each new
// NPC reservation in wirekind.h gets a row added below. Returns false when no
// sheet is shipped for that kind (caller falls back to the existing in-code
// rectangle placeholder).
struct NpcSheetRow {
  std::uint8_t kind;
  const char* slug;
};
inline constexpr NpcSheetRow kNpcSheets[] = {
    {content::kWireKindBonesmith,   "bonesmith_twins"},
    {content::kWireKindConfessor,   "confessor"},
    {content::kWireKindFence,       "cove_fence"},
    {content::kWireKindGuardAshen,  "guard_ashen"},
    {content::kWireKindGuardSynod,  "guard_synod"},
    {content::kWireKindRegistrar,   "pledge_registrar"},
    {content::kWireKindSteward,     "castle_steward"},
};
inline bool furnitureSheetPaths(std::uint8_t kind, char* png, std::size_t pngN,
                                char* js, std::size_t jsN) {
  if (!content::wireIsFurniture(kind)) return false;
  for (const auto& row : kNpcSheets) {
    if (row.kind == kind) {
      std::snprintf(png, pngN, "assets/aigen/npcs/%s/sheet.png", row.slug);
      std::snprintf(js, jsN, "assets/aigen/npcs/%s/sheet.json", row.slug);
      return true;
    }
  }
  return false;  // 64 Marta / 65 anvil / 66 bounty board still use in-code stubs
}

}  // namespace bh
