#pragma once

#include <cstddef>
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
  // T-064 boss kit (zeros on trash mobs)
  std::uint8_t boss;          // 1 = boss crowd markings; blood bolt enabled
  std::uint8_t boltRange;     // Blood Bolt cast range (tiles, chebyshev)
  std::uint16_t boltCdTicks;  // Blood Bolt cooldown @ 20 Hz
  // T-073 gate guard (default 0: every existing row stays a normal mob)
  std::uint8_t guard = 0;     // 1 = wanted-only aggro inside leash reach
};

// Sprint 7 additions: gnoll (L7) anchors mid fields, plague bat swarm (L2 fast,
// low dmg, packs) is the "die red-faced" exam question from the M2 criteria.
inline constexpr MobDef kMobs[] = {
    //  id     name             lv  hp   dmg  def  dex  xp    aggro cd  wand leash
    //                        loot id  %%   gold
    {1001, "Marsh Rat",        1,  30,  4,   2,   6,   40,   0,    16, 6,   12,  4001, 40,  6,   14,  0, 0, 0},
    {1002, "Feral Ghoul",      3,  64,  9,   5,   9,   90,   6,    18, 8,   14,  4002, 50,  14,  30,  0, 0, 0},
    {1003, "Hollow Hound",     5,  100, 13,  7,   12,  150,  7,    16, 10,  16,  4003, 60,  26,  48,  0, 0, 0},
    {1004, "Plague Bat",       2,  22,  5,   1,   14,  55,   8,    12, 12,  18,  4001, 25,  4,   12,  0, 0, 0},
    {1005, "Bonepicker Gnoll", 7,  160, 17,  10,  12,  300,  7,    20, 8,   14,  4003, 55,  60,  110,  0, 0, 0},
    {1006, "Charnel Widow",    9,  220, 22,  12,  14,  420,  6,    18, 6,   10,  4004, 35,  90,  160,  0, 0, 0},
    {1007, "Gravecaller",      11, 260, 30,  14,  15,  560,  7,    22, 5,   12,  4004, 40,  130, 220,  0, 0, 0},
    {1008, "Revenant Sexton",  12, 420, 34,  16,  15,  820,  8,    20, 4,   10,  4005, 100, 160, 260,  0, 0, 0},
    // T-064 S18 content drop: crypt elites (x8 of a same-level base) + the
    // Gravemother L14 boss (x20). Blood Bolt: ranged single-target cast at
    // 6 tiles on a 1.3s cycle, +25% potency at night (the deferred T-061 pin).
    {1009, "Gravemother",      14, 700, 40,  22,  15, 4000,  8,    24, 4,   14,  4005, 100, 400, 650,  1, 6, 26},
{1010, "Sepulcher Elite",  12, 380, 36,  18,  14, 3200,  8,    20, 4,   12,  4005, 100, 200, 320,  0, 0, 0},
    // T-073 gate guard: stands the east gate and the bridge approach. L15
    // wall of duty (hp 400 / dmg 30 / def 18), xp 0 (duty pays no purse),
    // aggro 0 + guard flag (wanted-only acquire inside leash reach, wander 0).
    {1011, "Gate Guard",       15, 400, 30,  18,  10,  0,    0,    20, 0,   12,  0,    0,   0,   0,    0, 0, 0,  1},
    // T-101 Old Maw (fields elite): Gnoll base through the 1010 pattern
    // (same L, hp ~0.9x, dmg/def +2, xp ~4x, richer gold).
    {1012, "Old Maw",           7, 145, 19,  12,  12, 1200,  7,    20, 8,   14,  4003, 55,  120, 220,  0, 0, 0},
};
inline constexpr size_t kMobKindCount = sizeof(kMobs) / sizeof(kMobs[0]);

// T-101 named elites (GDD §9: world-announced first-kill): index into the
// trio for session-scoped first-blood flags. -1 = not a named elite.
inline int namedEliteIdx(std::uint32_t mobId) {
  if (mobId == 1012) return 0;  // Old Maw (fields)
  if (mobId == 1013) return 1;  // Red Widow (mine)
  if (mobId == 1014) return 2;  // Cantor Vex (crypt)
  return -1;
}
// T-065 session-scoped bounty board: quarry cycles per 90 min of world time
// (era "the board wants..."), payout is the posted toll. No persistence,
// no in-place strategy churn — queue one chain, ring the bell, done.
struct BountyDef { std::uint32_t mobId; std::uint32_t payoutGold; };
inline constexpr BountyDef kBountyQuarry[] = {
    {1009, 1500},  // the Gravemother — the big purse
    {1007, 400},   // Gravecaller
    {1006, 300},   // Charnel Widow
    {1005, 250},   // Bonepicker Gnoll
};
inline constexpr std::uint64_t kBountyCycleTicks = 20 * 60 * 90;  // 90 min
inline const BountyDef* bountyAt(std::uint32_t cycle) {
  return &kBountyQuarry[cycle % (sizeof(kBountyQuarry) / sizeof(kBountyQuarry[0]))];
}

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
