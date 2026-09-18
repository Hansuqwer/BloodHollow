// ADR-0016 (T-159f1.1/.2): multi-affix items + 45-row per-band tables.
// Pins: 10-field blob round-trip + legacy tails, hasAffix across affix2/3,
// weapon/armor effect stacking, roll statute (Magic 1-2 / Rare 2-3 /
// distinct / Common 0), table law (45 rows, valid gear, valid mobs).
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

#include "content/items.h"
#include "content/mobs.h"
#include "world.h"

using namespace bh;

namespace {
sim::Map makeArena() {
  sim::Map m;
  m.w = 40;
  m.h = 40;
  m.tileW = 64;
  m.tileH = 32;
  m.ground.assign(40 * 40, 0);
  m.zone.assign(40 * 40, 0);
  m.blocked.assign(40 * 40, 0);
  return m;
}

server::Entity* spawnP(server::World& w, const char* name, int x, int y) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y});
  return w.find(e.id);
}

unsigned affixCount(const server::InvSlot& sl) {
  return (sl.affix > 0 ? 1u : 0u) + (sl.affix2 > 0 ? 1u : 0u) +
         (sl.affix3 > 0 ? 1u : 0u);
}
}  // namespace

TEST_CASE("ADR-0016: 10-field blob round-trips affix2/affix3") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "mule", 5, 5);
  server::InvSlot sl;
  sl.itemId = 2002;
  sl.qty = 1;
  sl.equipped = true;
  sl.affix = 1;
  sl.affix2 = 7;
  sl.affix3 = 11;
  sl.refine = 3;
  sl.rarity = 2;
  p->inv.push_back(sl);
  const std::string blob = server::canonicalInvBlob(p->inv);
  std::vector<server::InvSlot> out;
  server::parseInvBlob(blob, out);
  REQUIRE(out.size() == 1u);
  CHECK(out[0].affix == 1);
  CHECK(out[0].affix2 == 7);
  CHECK(out[0].affix3 == 11);
  CHECK(out[0].rarity == 2);
}

TEST_CASE("ADR-0016: legacy 8-field blobs default affix2/affix3 to 0") {
  std::vector<server::InvSlot> out;
  server::parseInvBlob("2002:1:1:0:100:1:3:2;", out);  // pre-ADR shape
  REQUIRE(out.size() == 1u);
  CHECK(out[0].affix == 1);
  CHECK(out[0].affix2 == 0);
  CHECK(out[0].affix3 == 0);
  CHECK(out[0].rarity == 2);
}

TEST_CASE("ADR-0016: hasAffix matches any of the three mods, slot-gated") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "wearer", 5, 5);
  server::InvSlot sl;
  sl.itemId = 2002;  // Pit Blade, slot 0
  sl.qty = 1;
  sl.equipped = true;
  sl.affix = 1;    // Whet
  sl.affix2 = 7;   // Embers
  sl.affix3 = 16;  // Boneyard
  p->inv.push_back(sl);
  CHECK(w.hasAffix(*p, 1, 0));
  CHECK(w.hasAffix(*p, 7, 0));
  CHECK(w.hasAffix(*p, 16, 0));
  CHECK_FALSE(w.hasAffix(*p, 2, 0));  // Warding is armor-only here
  CHECK_FALSE(w.hasAffix(*p, 1, 1));  // slot gate holds
  CHECK(w.hasAffix(*p, 7, 9));        // any-gear lane
}

TEST_CASE("ADR-0016: weapon mods stack across fields (Whet + Embers)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  w.debugSetTick(500);  // morning: no night bonus in the expectation
  server::Entity* p = spawnP(w, "stacker", 5, 5);
  server::InvSlot sl;
  sl.itemId = 2002;  // Pit Blade dmg 18
  sl.qty = 1;
  sl.equipped = true;
  sl.affix = 1;    // +10%: 18 + 1
  sl.affix2 = 7;   // +2 flat
  p->inv.push_back(sl);
  // 18 + 18/10 + 2 = 21
  CHECK(w.debugWeaponDmg(*p) == 21u);
}

TEST_CASE("ADR-0016: armor mods stack across fields (Warding + Husk)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "tank", 5, 5);
  server::InvSlot sl;
  sl.itemId = 2102;  // Bone Plate def 11
  sl.qty = 1;
  sl.equipped = true;
  sl.affix = 2;    // +2
  sl.affix2 = 18;  // +2
  p->inv.push_back(sl);
  CHECK(w.equippedArmorDef(*p) == 15u);
}

TEST_CASE("ADR-0016: roll statute over 2000 ghoul kills") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* k = spawnP(w, "slayer", 5, 5);
  k->level = 10;
  k->hp = 1000000;
  k->hpMax = 1000000;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  unsigned magic = 0, rare = 0, common = 0;
  for (int i = 0; i < 2000; ++i) {
    server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{6, 5});
    w.debugKillMob(m, k);
    for (const server::InvSlot& sl : k->inv) {
      const content::ItemDef* d = content::findItem(sl.itemId);
      if (d == nullptr || d->slot > 4) continue;  // gear only
      const unsigned n = affixCount(sl);
      if (sl.rarity == content::kRarityCommon) {
        ++common;
        CHECK(n == 0u);
      } else if (sl.rarity == content::kRarityMagic) {
        ++magic;
        CHECK(n >= 1u);
        CHECK(n <= 2u);
      } else if (sl.rarity == content::kRarityRare) {
        ++rare;
        CHECK(n >= 2u);
        CHECK(n <= 3u);
      }
      // distinctness: no doubled mod on one item
      if (sl.affix != 0) {
        CHECK(sl.affix2 != sl.affix);
        CHECK(sl.affix3 != sl.affix);
      }
      if (sl.affix2 != 0) CHECK(sl.affix3 != sl.affix2);
      for (const std::uint8_t ax : {sl.affix, sl.affix2, sl.affix3})
        CHECK(ax <= content::kAffixCount);
    }
    // strip gear so the 32-slot cap never gates the statute
    std::vector<server::InvSlot> kept;
    for (const server::InvSlot& sl : k->inv) {
      const content::ItemDef* d = content::findItem(sl.itemId);
      if (d == nullptr || d->slot > 4) kept.push_back(sl);
    }
    k->inv = kept;
  }
  CHECK(common > 0u);  // Commons still drop bare
  CHECK(magic > 0u);   // ~17% of gear over 400 kills
  CHECK(rare > 0u);    // ~4.6% of gear over 400 kills
}

TEST_CASE("ADR-0016: band table law — 45 rows, live gear, live mobs") {
  CHECK(content::kGearDropCount == 45u);
  for (std::uint32_t i = 0; i < content::kGearDropCount; ++i) {
    const content::GearDropDef& g = content::kGearDrops[i];
    const content::ItemDef* d = content::findItem(g.itemId);
    REQUIRE(d != nullptr);
    CHECK(d->slot <= 4);     // gear only: never junk/consumable
    CHECK(d->stackMax == 1);  // unstackable equips
    CHECK(g.chancePct >= 1);
    CHECK(g.chancePct <= 5);  // band-table economy ceiling
    CHECK(content::findMob(g.mobId) != nullptr);
  }
}
