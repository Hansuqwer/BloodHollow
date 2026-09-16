// Boss uniques (H3 loot depth). T-127 seeds Old Maw (1012); T-128..T-130
// append Widow (1013) / Cantor (1014) / Gravemother rows to the same table.
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
