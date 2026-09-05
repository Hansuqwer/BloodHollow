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
    {4001, "Rat Pelt",             3, 0,  0,  0,   10,   32},
    {4002, "Ghoul Finger",         3, 0,  0,  0,   22,   32},
    {4003, "Hound Fang",           3, 0,  0,  0,   35,   32},
    {4004, "Widow Silk",           3, 0,  0,  0,   55,   32},
    {4005, "Revenant Ash",         3, 0,  0,  0,   75,   16},
};

inline const ItemDef* findItem(std::uint32_t itemId) {
  for (const ItemDef& d : kItems) {
    if (d.itemId == itemId) return &d;
  }
  return nullptr;
}

// Town vendor stock (Marta). Buy = full value; SellJunk = 40%.
inline constexpr std::uint32_t kVendorStock[] = {2001, 2002, 2101, 2102, 3001};

}  // namespace bh::content
