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
    {2101, "Hide Armor",           1, 0,  6,  0,   120,  1},
    {2102, "Bone Plate",           1, 0,  11, 0,   350,  1},
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

// ---- T-059 affixes v1 -----------------------------------------------
// drop-time one-liner mods; era-small, stack-free (one per item).
// 1 whet (+10% weapon dmg), 2 ward (+2 armor def), 3 leech (+5% of dealt dmg healed)
inline constexpr const char* kAffixNames[] = {"", "of Whet", "of Warding", "of Leech"};
inline constexpr std::uint8_t kAffixCount = 3;

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
