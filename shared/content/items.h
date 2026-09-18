#pragma once

#include <cstdint>

namespace bh::content {

// Rarity tier constants (T-159).
inline constexpr std::uint8_t kRarityCommon = 0;
inline constexpr std::uint8_t kRarityMagic  = 1;
inline constexpr std::uint8_t kRarityRare   = 2;
inline constexpr std::uint8_t kRarityUnique = 3;
inline constexpr std::uint8_t kRarityCount  = 4;

// Item/equipment content table (MVP slice). slot: 0=weapon, 1=armor,
// 2=helm, 3=amulet, 4=ring, 5=consumable, 6=junk-loot (no equip).
// value = vendor gold price; sells at kSellRatio for junk/anything (fence
// rules for chaotics land in P3). rarity: 0 common (default), 3 unique
// (boss-only fixed drops).
struct ItemDef {
  std::uint32_t itemId;
  const char* name;
  std::uint8_t slot;       // 0 weapon, 1 armor, 2 helm, 3 amulet, 4 ring, 5 consumable, 6 junk
  std::uint32_t dmg;       // weapon: base damage (replaces fists)
  std::uint32_t def;       // armor/helm: DEF term
  std::uint32_t heal;      // consumable: instant HP
  std::uint32_t value;     // gold
  std::uint16_t stackMax;  // 1 = unstackable (equips)
  std::uint8_t rarity = 0; // T-159: 0 common, 3 unique
};

inline constexpr std::uint32_t kSellRatioPct = 40;  // sell at 40% of value

inline constexpr ItemDef kItems[] = {
    //  id    name                slot dmg def heal value stack rarity
    // weapons (slot 0)
    {2001, "Rusty Shank",          0, 12, 0,  0,   80,   1, 0},
    {2002, "Pit Blade",            0, 18, 0,  0,   260,  1, 0},
    {2003, "Mine Pick",            0, 8,  0,  0,   150,  1, 0},
    // armor (slot 1)
    {2101, "Hide Armor",           1, 0,  6,  0,   120,  1, 0},
    {2102, "Bone Plate",           1, 0,  11, 0,   350,  1, 0},
    // helm (slot 2)
    {2501, "Scrap Helm",           2, 0,  3,  0,   90,   1, 0},
    {2502, "Graveguard Helm",      2, 0,  7,  0,   280,  1, 0},
    {2503, "Hollow Warden",        2, 0,  10, 0,   500,  1, 0},
    // amulet (slot 3)
    {2601, "Bone Charm",           3, 0,  0,  0,   100,  1, 0},
    {2602, "Grave Lodestone",      3, 0,  0,  0,   320,  1, 0},
    {2603, "Marrow Talisman",      3, 0,  0,  0,   550,  1, 0},
    // ring (slot 4)
    {2701, "Iron Band",            4, 0,  0,  0,   70,   1, 0},
    {2702, "Ossuary Ring",         4, 0,  0,  0,   240,  1, 0},
    {2703, "Seal of the Hollow",   4, 0,  0,  0,   480,  1, 0},
    // T-127 Old Maw uniques (found, never stocked): Mawsplitter 2201,
    // Gullet Plate 2103, Mawfang Shiv 2202.
    {2201, "Mawsplitter",          0, 22, 0,  0,   600,  1, 3},
    {2103, "Gullet Plate",         1, 0,  13, 0,   700,  1, 3},
    {2202, "Mawfang Shiv",         0, 15, 0,  0,   450,  1, 3},
    // T-128 trio uniques: Widow 2301/2104/2302, Cantor 2303/2105/2304,
    // Gravemother 2401/2106/2402.
    {2301, "Widow's Needle",       0, 20, 0,  0,   900,  1, 3},
    {2104, "Silkwoven Shroud",     1, 0,  12, 0,   850,  1, 3},
    {2302, "Red Widow's Kiss",     0, 17, 0,  0,   800,  1, 3},
    {2303, "Cantor's Quill",       0, 19, 0,  0,   1100, 1, 3},
    {2105, "Vigil Cope",           1, 0,  12, 0,   1050, 1, 3},
    {2304, "Vex Nail",             0, 16, 0,  0,   1000, 1, 3},
    {2401, "Tithehook",            0, 28, 0,  0,   1800, 1, 3},
    {2106, "Sepulcher Plate",      1, 0,  16, 0,   1700, 1, 3},
    {2402, "Caulblade",            0, 24, 0,  0,   1600, 1, 3},
    // consumable (slot 5)
    {3001, "Blood Vial",           5, 0,  0,  40,  30,   16, 0},
    {3002, "Smuggled Vial",        5, 0,  0,  55,  45,   16, 0},
    // T-071 night light: torches burn out (timed), the lantern never does
    // while held (toggle). heal=0: light, not life — useItem branches by id.
    {3003, "Torch",                5, 0,  0,  0,   8,    8,  0},
    {3004, "Blessed Lantern",      5, 0,  0,  0,   150,  1, 0},
    // junk (slot 6)
    {4001, "Rat Pelt",             6, 0,  0,  0,   10,   32, 0},
    {4002, "Ghoul Finger",         6, 0,  0,  0,   22,   32, 0},
    {4003, "Hound Fang",           6, 0,  0,  0,   35,   32, 0},
    {4004, "Widow Silk",           6, 0,  0,  0,   55,   32, 0},
    {4005, "Revenant Ash",         6, 0,  0,  0,   75,   16, 0},
    {5001, "Blackiron Ore",        6, 0,  0,  0,   20,   32, 0},
};

// ---- T-059 affixes v1 + T-126 affix v2 + T-159 affix v3 ---------------
// drop-time one-liner mods; era-small, stack-free (one per item).
// v1: 1 whet (+10% weapon dmg), 2 ward (+2 armor def), 3 leech (+5% of dealt dmg healed)
// v2: 4 ox (+20 hpMax, armor), 5 thorns (reflect 2, never kills, armor),
// 6 focus (+4 acc, weapon), 7 embers (+2 dmg +1 at night, weapon),
// 8 vigil (+2 light radius while lit, any gear), 9 greed (+10% kill gold, weapon),
// 10 mending (+1 OOC regen, armor).
// v3 (T-159): 11 hollow (+3 flat dmg, weapon), 12 grave-touched (+1 OOC regen, any gear),
// 13 cryptward (-10% incoming dmg, armor), 14 marrow (+3% lifesteal, weapon),
// 15 pall (+2 acc +1 evd, any gear), 16 boneyard (+5% crit chance, weapon),
// 17 dirge (+4 dmg at night, weapon), 18 husk (+2 flat def, armor),
// 19 tithemaster (+15% kill gold, weapon), 20 last rites (+8 dmg <20% hp, weapon).
// Wrong slot = flavor text only (v1 precedent).
inline constexpr const char* kAffixNames[] = {"", "of Whet", "of Warding", "of Leech",
    "of the Ox", "of Thorns", "of Focus", "of Embers", "of the Vigil", "of Greed",
    "of Mending", "of the Hollow", "Grave-touched", "of the Crypt", "of the Marrow",
    "of the Pall", "of the Boneyard", "of the Dirge", "of the Husk", "of the Tithemaster",
    "of Last Rites"};
inline constexpr std::uint8_t kAffixCount = 20;

// gear-drop side-table (kept off MobDef rows: content table stays 16-wide)
// T-159f1.2 (ADR-0016): 45 rows banded by mob level — L1-3 light gear,
// L5-8 pit gear, L9-11 warden gear, L12+ grave gear. Chances 2-5% (the
// gear-chance gate + rarity roll are unchanged, so the economy impact is
// bounded: more KINDS drop, not more drops).
struct GearDropDef { std::uint32_t mobId; std::uint32_t itemId; std::uint8_t chancePct; };
inline constexpr GearDropDef kGearDrops[] = {
    // L1-3 band: rats, bats, ghouls — shanks, hide, scrap, bands, charms
    {1001, 2001, 4},   // Marsh Rat: Rusty Shank
    {1001, 2701, 3},   // Marsh Rat: Iron Band
    {1004, 2001, 4},   // Plague Bat: Rusty Shank
    {1004, 2601, 2},   // Plague Bat: Bone Charm
    {1002, 2001, 4},   // Feral Ghoul: Rusty Shank
    {1002, 2101, 3},   // Feral Ghoul: Hide Armor
    {1002, 2501, 2},   // Feral Ghoul: Scrap Helm
    // L5-8 band: hounds, gnolls, wretches, spiders — pit gear + first warden
    {1003, 2001, 3},   // Hollow Hound: Rusty Shank
    {1003, 2101, 3},   // Hollow Hound: Hide Armor
    {1003, 2701, 3},   // Hollow Hound: Iron Band
    {1016, 2601, 3},   // Lantern Spider: Bone Charm
    {1005, 2002, 3},   // Bonepicker Gnoll: Pit Blade
    {1005, 2101, 3},   // Bonepicker Gnoll: Hide Armor
    {1005, 2702, 2},   // Bonepicker Gnoll: Ossuary Ring
    {1012, 2002, 4},   // Old Maw: Pit Blade
    {1012, 2101, 3},   // Old Maw: Hide Armor
    {1015, 2002, 3},   // Mine Wretch: Pit Blade
    {1015, 2101, 3},   // Mine Wretch: Hide Armor
    {1022, 2601, 3},   // Wraith: Bone Charm
    // L9-11 band: widows, golems, fiends, patrols — bone plate + warden gear
    {1006, 2101, 3},   // Charnel Widow: Hide Armor
    {1006, 2102, 2},   // Charnel Widow: Bone Plate
    {1006, 2502, 2},   // Charnel Widow: Graveguard Helm
    {1013, 2102, 3},   // Red Widow: Bone Plate
    {1013, 2502, 2},   // Red Widow: Graveguard Helm
    {1013, 2602, 2},   // Red Widow: Grave Lodestone
    {1017, 2102, 2},   // Mud Golem: Bone Plate
    {1017, 2702, 2},   // Mud Golem: Ossuary Ring
    {1023, 2002, 3},   // Bloodfiend: Pit Blade
    {1023, 2602, 2},   // Bloodfiend: Grave Lodestone
    {1025, 2002, 3},   // Ashen Patrol: Pit Blade
    {1024, 2002, 3},   // Synod Patrol: Pit Blade
    {1021, 2102, 2},   // Pale Cultist: Bone Plate
    // L11-12 band: celebrants, sextons, revenants — gravelodestone tier
    {1007, 2102, 2},   // Waxen Celebrant: Bone Plate
    {1007, 2602, 2},   // Waxen Celebrant: Grave Lodestone
    {1007, 2702, 2},   // Waxen Celebrant: Ossuary Ring
    {1008, 2102, 3},   // Revenant Sexton: Bone Plate
    {1008, 2502, 2},   // Revenant Sexton: Graveguard Helm
    {1018, 2102, 2},   // Crypt Revenant: Bone Plate
    // L12+ band: elites, banshees, cantors, ringers — hollow warden tier
    {1010, 2503, 2},   // Sepulcher Elite: Hollow Warden
    {1010, 2603, 2},   // Sepulcher Elite: Marrow Talisman
    {1010, 2703, 2},   // Sepulcher Elite: Seal of the Hollow
    {1019, 2503, 2},   // Grave Banshee: Hollow Warden
    {1014, 2603, 2},   // Cantor Vex: Marrow Talisman
    {1014, 2703, 2},   // Cantor Vex: Seal of the Hollow
    {1020, 2503, 2},   // Bell Ringer: Hollow Warden
};
inline constexpr std::uint32_t kGearDropCount =
    sizeof(kGearDrops) / sizeof(kGearDrops[0]);
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
// T-159: vendor now stocks helm (2501/2502), amulet (2601/2602), ring (2701/2702).
inline constexpr std::uint32_t kVendorStock[] = {2001, 2002, 2003, 2101, 2102,
    2501, 2502, 2601, 2602, 2701, 2702, 3001, 3003, 3004};

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
