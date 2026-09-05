#pragma once

#include <cstdint>

#include "content/wirekind.h"

namespace bh::content {

// Monster definitions (content, not engine). mobId keyed; the map's spawner
// table (make_thornwall.py) references these ids. Deterministic and
// dependency-free so both the server sim and tests share one table.
struct MobDef {
  std::uint32_t mobId;
  const char* name;
  std::uint8_t level;
  std::uint32_t hp;
  std::uint32_t dmg;      // raw weapon-equivalent before STR scaling (mobs don't scale)
  std::uint32_t def;
  std::uint8_t dex;       // drives ACC=2*dex / EVD=dex like players
  std::uint32_t xp;       // flat kill XP (GDD: elites x8 etc. layered later)
  std::uint8_t aggroRadius;   // tiles; 0 = passive (retaliate only)
  std::uint16_t atkCdTicks;   // swing cooldown @ 20 Hz
  std::uint8_t wanderRadius;  // tiles around anchor
  std::uint8_t leashRadius;   // de-aggro + walk home + full heal
  std::uint32_t lootItemId;   // 0 = none (junk drop, content/items.h)
  std::uint8_t lootChancePct; // roll on kill
  std::uint32_t goldLo, goldHi;  // gold drop range
};

// Sprint 7 additions: gnoll (L7) anchors mid fields, plague bat swarm (L2 fast,
// low dmg, packs) is the "die red-faced" exam question from the M2 criteria.
inline constexpr MobDef kMobs[] = {
    //  id     name             lv  hp   dmg  def  dex  xp    aggro cd  wand leash
    //                        loot id  %%   gold
    {1001, "Marsh Rat",        1,  30,  4,   2,   6,   40,   0,    16, 6,   12,  4001, 40,  6,   14},
    {1002, "Feral Ghoul",      3,  64,  9,   5,   9,   90,   6,    18, 8,   14,  4002, 50,  14,  30},
    {1003, "Hollow Hound",     5,  100, 13,  7,   12,  150,  7,    16, 10,  16,  4003, 60,  26,  48},
    {1004, "Plague Bat",       2,  22,  5,   1,   14,  55,   8,    12, 12,  18,  4001, 25,  4,   12},
    {1005, "Bonepicker Gnoll", 7,  160, 18,  10,  12,  300,  7,    20, 8,   14,  4003, 55,  60,  110},
    {1006, "Charnel Widow",    9,  220, 24,  12,  14,  420,  6,    18, 6,   12,  4004, 35,  90,  160},
    {1007, "Gravecaller",      11, 260, 30,  14,  15,  560,  7,    22, 5,   12,  4004, 40,  130, 220},
    {1008, "Revenant Sexton",  12, 420, 34,  16,  15,  820,  8,    20, 4,   10,  4005, 100, 160, 260},
};
inline constexpr size_t kMobKindCount = sizeof(kMobs) / sizeof(kMobs[0]);
// wireKind = 1-based index into kMobs (world.cpp spawnMob); furniture band
// starts at 64, so the mob table must never reach it (T-047 invariant).
static_assert(kMobKindCount < content::kWireKindFurnitureFloor,
              "mob table collides with the wire furniture band (64+)");


inline const MobDef* findMob(std::uint32_t mobId) {
  for (const MobDef& d : kMobs) {
    if (d.mobId == mobId) return &d;
  }
  return nullptr;
}

}  // namespace bh::content
