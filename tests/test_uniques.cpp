// Boss uniques (H3 loot depth). T-127 seeds Old Maw (1012); T-128 appends
// Widow (1013) / Cantor (1014) / Gravemother (1009) — 12/12 MVP set.
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

#include "content/items.h"
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
}  // namespace

TEST_CASE("T-127: Old Maw holds exactly 3 unique rows") {
  std::uint32_t maw = 0;
  for (std::uint32_t i = 0; i < content::kUniqueDropCount; ++i) {
    const content::UniqueDropDef& u = content::kUniqueDrops[i];
    if (u.mobId != 1012) continue;
    ++maw;
    // fixed affix inside the v2 table, item resolves, title non-empty
    CHECK(u.affix >= 1u);
    CHECK(u.affix <= content::kAffixCount);
    const content::ItemDef* id = content::findItem(u.itemId);
    REQUIRE(id != nullptr);
    CHECK(std::string(u.title).size() > 0);
    CHECK(u.chancePct == 4u);
  }
  CHECK(maw == 3u);
}

TEST_CASE("T-127: new unique items resolve with era weights") {
  const content::ItemDef* splitter = content::findItem(2201);
  REQUIRE(splitter != nullptr);
  CHECK(splitter->slot == 0u);
  CHECK(splitter->dmg == 22u);
  const content::ItemDef* gullet = content::findItem(2103);
  REQUIRE(gullet != nullptr);
  CHECK(gullet->slot == 1u);
  CHECK(gullet->def == 13u);
  const content::ItemDef* shiv = content::findItem(2202);
  REQUIRE(shiv != nullptr);
  CHECK(shiv->slot == 0u);
  CHECK(shiv->dmg == 15u);
  // found, never stocked: neither vendor lane sells uniques
  for (const std::uint32_t id : content::kVendorStock) {
    CHECK(id != 2201u);
    CHECK(id != 2103u);
    CHECK(id != 2202u);
  }
  for (const std::uint32_t id : content::kFenceStock) {
    CHECK(id != 2201u);
    CHECK(id != 2103u);
    CHECK(id != 2202u);
  }
}

TEST_CASE("T-127: grantUniqueDrop delivers fixed item+affix+broadcast") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "mawslayer", 5, 5);
  const content::UniqueDropDef& u = content::kUniqueDrops[0];  // Mawsplitter
  REQUIRE(u.mobId == 1012u);
  CHECK(w.grantUniqueDrop(*p, u));
  REQUIRE(p->inv.size() == 1u);
  CHECK(p->inv[0].itemId == 2201u);
  CHECK(p->inv[0].affix == 7u);  // fixed of Embers, never rolled
  CHECK(p->inv[0].qty == 1u);
  CHECK_FALSE(p->inv[0].equipped);
  bool herald = false;
  for (const auto& ev : w.events()) {
    if (ev.chatCh == 2 &&
        ev.chatText.find("claims Mawsplitter") != std::string::npos &&
        ev.chatText.find("Tooth of the Pit") != std::string::npos)
      herald = true;
  }
  CHECK(herald);
}

TEST_CASE("T-127: inv-full grants nothing, no partial state") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "packrat", 5, 5);
  for (int i = 0; i < 40; ++i) w.debugGive(*p, 2001, 1);  // unstackable: fills slots
  REQUIRE(p->inv.size() == 32u);  // inventory cap holds
  const std::size_t events0 = w.events().size();
  CHECK_FALSE(w.grantUniqueDrop(*p, content::kUniqueDrops[1]));
  CHECK(p->inv.size() == 32u);
  CHECK(w.events().size() == events0);  // no broadcast without a grant
}

// ---- T-128 trio: Widow (1013) / Cantor (1014) / Gravemother (1009) ----
TEST_CASE("T-128: per-boss row counts and rates") {
  std::uint32_t widow = 0, cantor = 0, mother = 0;
  for (std::uint32_t i = 0; i < content::kUniqueDropCount; ++i) {
    const content::UniqueDropDef& u = content::kUniqueDrops[i];
    // every row: legal fixed affix, resolving item, titled
    CHECK(u.affix >= 1u);
    CHECK(u.affix <= content::kAffixCount);
    REQUIRE(content::findItem(u.itemId) != nullptr);
    CHECK(std::string(u.title).size() > 0);
    // slot law: armor rows 2/4/5/8/10, weapon rows 1/3/6/7/9
    const content::ItemDef* id = content::findItem(u.itemId);
    if (id->slot == 1) {
      CHECK((u.affix == 2u || u.affix == 4u || u.affix == 5u || u.affix == 8u ||
             u.affix == 10u));
    } else {
      CHECK((u.affix == 1u || u.affix == 3u || u.affix == 6u || u.affix == 7u ||
             u.affix == 9u));
    }
    if (u.mobId == 1013) {
      ++widow;
      CHECK(u.chancePct == 4u);
    }
    if (u.mobId == 1014) {
      ++cantor;
      CHECK(u.chancePct == 4u);
    }
    if (u.mobId == 1009) {
      ++mother;
      CHECK(u.chancePct == 6u);
    }
  }
  CHECK(widow == 3u);
  CHECK(cantor == 3u);
  CHECK(mother == 3u);
  CHECK(content::kUniqueDropCount == 12u);  // 12/12 MVP set complete
}

TEST_CASE("T-128: trio items resolve with boss-tier weights") {
  CHECK(content::findItem(2301)->dmg == 20u);  // Widow's Needle
  CHECK(content::findItem(2104)->def == 12u);  // Silkwoven Shroud
  CHECK(content::findItem(2302)->dmg == 17u);  // Red Widow's Kiss
  CHECK(content::findItem(2303)->dmg == 19u);  // Cantor's Quill
  CHECK(content::findItem(2105)->def == 12u);  // Vigil Cope
  CHECK(content::findItem(2304)->dmg == 16u);  // Vex Nail
  CHECK(content::findItem(2401)->dmg == 28u);  // Tithehook (boss)
  CHECK(content::findItem(2106)->def == 16u);  // Sepulcher Plate (boss)
  CHECK(content::findItem(2402)->dmg == 24u);  // Caulblade (boss)
  // found, never stocked: sweep all 9 against both lanes
  const std::uint32_t ids[] = {2301, 2104, 2302, 2303, 2105,
                               2304, 2401, 2106, 2402};
  for (const std::uint32_t want : ids) {
    for (const std::uint32_t id : content::kVendorStock) CHECK(id != want);
    for (const std::uint32_t id : content::kFenceStock) CHECK(id != want);
  }
}

TEST_CASE("T-128: one grant spot-check per trio boss") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "trio", 5, 5);
  // Widow row 3, Cantor row 6, Mother row 9 (table order: Maw 0-2, Widow 3-5,
  // Cantor 6-8, Mother 9-11)
  const std::uint32_t rows[] = {3, 6, 9};
  const std::uint32_t items[] = {2301, 2303, 2401};
  const std::uint8_t affixes[] = {6, 7, 9};
  for (int k = 0; k < 3; ++k) {
    const content::UniqueDropDef& u = content::kUniqueDrops[rows[k]];
    REQUIRE(w.grantUniqueDrop(*p, u));
    CHECK(p->inv.back().itemId == items[k]);
    CHECK(p->inv.back().affix == affixes[k]);
  }
  bool needle = false, quill = false, tithe = false;
  for (const auto& ev : w.events()) {
    if (ev.chatCh != 2) continue;
    if (ev.chatText.find("Widow's Needle") != std::string::npos) needle = true;
    if (ev.chatText.find("Cantor's Quill") != std::string::npos) quill = true;
    if (ev.chatText.find("Tithehook") != std::string::npos) tithe = true;
  }
  CHECK(needle);
  CHECK(quill);
  CHECK(tithe);
}
