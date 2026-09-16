#pragma once

#include <cstdint>

namespace bh::content {

// Item/equipment content table (MVP slice). slot: 0=weapon, 1=armor,
// 2=consumable, 3=junk-loot (no equip). value = vendor gold price; sells at
// kSellRatio for junk/anything (fence rules for chaotics land in P3).
struct ItemDef {
  std::uint32_t itemId;
  const char* name;
  std::uint8_t slot;       // 0 weapon, 1 armor, 2 consumable, 3 junk
  std::uint32_t dmg;       // weapon: base damage (replaces fists)
  std::uint32_t def;       // armor: DEF term
  std::uint32_t heal;      // consumable: instant HP
  std::uint32_t value;     // gold
  std::uint16_t stackMax;  // 1 = unstackable (equips)
};

inline constexpr std::uint32_t kSellRatioPct = 40;  // sell at 40% of value

inline constexpr ItemDef kItems[] = {
    //  id    name                slot dmg def heal value stack
    {2001, "Rusty Shank",          0, 12, 0,  0,   80,   1},
    {2002, "Pit Blade",            0, 18, 0,  0,   260,  1},
    {2003, "Mine Pick",            0, 8,  0,  0,   150,  1},
    {2101, "Hide Armor",           1, 0,  6,  0,   120,  1},
    {2102, "Bone Plate",           1, 0,  11, 0,   350,  1},
    // T-127 Old Maw uniques (found, never stocked): Mawsplitter 2201,
    // Gullet Plate 2103, Mawfang Shiv 2202.
    {2201, "Mawsplitter",          0, 22, 0,  0,   600,  1},
    {2103, "Gullet Plate",         1, 0,  13, 0,   700,  1},
    {2202, "Mawfang Shiv",         0, 15, 0,  0,   450,  1},
    // T-128 trio uniques: Widow 2301/2104/2302, Cantor 2303/2105/2304,
    // Gravemother 2401/2106/2402.
    {2301, "Widow's Needle",       0, 20, 0,  0,   900,  1},
    {2104, "Silkwoven Shroud",     1, 0,  12, 0,   850,  1},
    {2302, "Red Widow's Kiss",     0, 17, 0,  0,   800,  1},
    {2303, "Cantor's Quill",       0, 19, 0,  0,   1100, 1},
    {2105, "Vigil Cope",           1, 0,  12, 0,   1050, 1},
    {2304, "Vex Nail",             0, 16, 0,  0,   1000, 1},
    {2401, "Tithehook",            0, 28, 0,  0,   1800, 1},
    {2106, "Sepulcher Plate",      1, 0,  16, 0,   1700, 1},
    {2402, "Caulblade",            0, 24, 0,  0,   1600, 1},
    {3001, "Blood Vial",           2, 0,  0,  40,  30,   16},
    {3002, "Smuggled Vial",        2, 0,  0,  55,  45,   16},
    // T-071 night light: torches burn out (timed), the lantern never does
    // while held (toggle). heal=0: light, not life — useItem branches by id.
    {3003, "Torch",                2, 0,  0,  0,   8,    8},
    {3004, "Blessed Lantern",      2, 0,  0,  0,   150,  1},
    {4001, "Rat Pelt",             3, 0,  0,  0,   10,   32},
    {4002, "Ghoul Finger",         3, 0,  0,  0,   22,   32},
    {4003, "Hound Fang",           3, 0,  0,  0,   35,   32},
    {4004, "Widow Silk",           3, 0,  0,  0,   55,   32},
    {4005, "Revenant Ash",         3, 0,  0,  0,   75,   16},
};

// ---- T-059 affixes v1 + T-126 affix v2 -------------------------------
// drop-time one-liner mods; era-small, stack-free (one per item).
// 1 whet (+10% weapon dmg), 2 ward (+2 armor def), 3 leech (+5% of dealt dmg healed)
// T-126 v2: 4 ox (+20 hpMax, armor), 5 thorns (reflect 2, never kills, armor),
// 6 focus (+4 acc, weapon), 7 embers (+2 dmg +1 at night, weapon),
// 8 vigil (+2 light radius while lit, any gear), 9 greed (+10% kill gold, weapon),
// 10 mending (+1 OOC regen, armor). Wrong slot = flavor text only (v1 precedent).
inline constexpr const char* kAffixNames[] = {"", "of Whet", "of Warding", "of Leech",
    "of the Ox", "of Thorns", "of Focus", "of Embers", "of the Vigil", "of Greed",
    "of Mending"};
inline constexpr std::uint8_t kAffixCount = 10;

// gear-drop side-table (kept off MobDef rows: content table stays 16-wide)
struct GearDropDef { std::uint32_t mobId; std::uint32_t itemId; std::uint8_t chancePct; };
inline constexpr GearDropDef kGearDrops[] = {
    {1002, 2001, 4},   // Feral Ghoul: Rusty Shank
    {1005, 2002, 3},   // Bonepicker Gnoll: Pit Blade
    {1006, 2101, 3},   // Charnel Widow: Hide Armor
    {1007, 2102, 2},   // Gravecaller: Bone Plate
};
inline const GearDropDef* findGearDrop(std::uint32_t mobId) {
  for (const auto& g : kGearDrops) if (g.mobId == mobId) return &g;
  return nullptr;
}

// ---- T-127 boss uniques ------------------------------------------------
// Fixed item + fixed affix + title; elites roll each row independently at
// chancePct (night rides +25% like all drops, T-062). T-127 seeds Old Maw
// (1012); T-128 appends Widow (1013) / Cantor (1014) / Gravemother (1009).
// Slot law: armor rows use 2/4/5/8/10, weapon rows 1/3/6/7/9 (a mismatched
// fixed affix would be mute text — checked by test_uniques slot pins).
struct UniqueDropDef {
  std::uint32_t mobId;
  std::uint32_t itemId;
  std::uint8_t affix;      // fixed, never rolled (must be <= kAffixCount)
  std::uint8_t chancePct;  // per-row independent roll
  const char* title;       // broadcast epithet, non-empty
};
inline constexpr UniqueDropDef kUniqueDrops[] = {
    {1012, 2201, 7, 4, "Tooth of the Pit"},
    {1012, 2103, 5, 4, "The Maw That Keeps"},
    {1012, 2202, 9, 4, "Tithetaker"},
    // T-128 Red Widow (1013, mine nest) @4%
    {1013, 2301, 6, 4, "The Huntress Answers"},
    {1013, 2104, 10, 4, "Woven From Hunger"},
    {1013, 2302, 3, 4, "Drink, Dear"},
    // T-128 Cantor Vex (1014, crypt choir) @4%
    {1014, 2303, 7, 4, "The Last Verse Burns"},
    {1014, 2105, 8, 4, "The Choir Keeps Watch"},
    {1014, 2304, 1, 4, "Keen As Doctrine"},
    // T-128 Gravemother (1009, L14 boss) @6% (boss judgment call)
    {1009, 2401, 9, 6, "The Mother Collects"},
    {1009, 2106, 4, 6, "Born Heavy"},
    {1009, 2402, 3, 6, "First Blood, Again"},
};
inline constexpr std::uint32_t kUniqueDropCount =
    sizeof(kUniqueDrops) / sizeof(kUniqueDrops[0]);

inline const ItemDef* findItem(std::uint32_t itemId) {
  for (const ItemDef& d : kItems) {
    if (d.itemId == itemId) return &d;
  }
  return nullptr;
}

// Town vendor stock (Marta). Buy = full value; SellJunk = 40%.
// T-071: Marta stocks the night (torch 8g, lantern 150g — pinned by the
// overnight shift; director review). Potions/gold numbers otherwise untouched.
inline constexpr std::uint32_t kVendorStock[] = {2001, 2002, 2101, 2102, 3001,
                                                 3003, 3004};

// T-069 Smugglers' Cove fence (Sable): the no-questions lane Marta refuses.
// Secret stock = contraband potion + rare junk, chaotic eyes only, at a 25%
// markup; junk pawn pays 60% to anyone — better than Marta's 40%, that's the
// draw. Stock is item-DISJOINT from kVendorStock so the kBuy lane can route
// by item (fence stock -> Sable, everything else -> Marta) with no ambiguity.
inline constexpr std::uint32_t kFenceStock[] = {3002, 4004, 4005};
inline constexpr std::uint32_t kFenceMarkupPct = 125;
inline constexpr std::uint32_t kFenceSellRatioPct = 60;
inline bool isFenceStock(std::uint32_t itemId) {
  for (const std::uint32_t id : kFenceStock) if (id == itemId) return true;
  return false;
}

}  // namespace bh::content
