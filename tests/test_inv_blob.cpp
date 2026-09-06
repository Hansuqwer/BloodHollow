// T-049x (relog-launderer fix): the ONE inventory-blob grammar shared by the
// live login path and world-replay applyLogin. Pre-fix, replay parsed a
// 4-field sscanf + debugGive lane that resurrected dormant gear (durability
// 0 -> 100), dropped affix/refine, and scrambled slot order via stacking.
// These cases pin the grammar so the two paths can never drift again.
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "world.h"

using namespace bh;
using bh::server::InvSlot;

TEST_CASE("inv blob: full 7-field records round-trip field-for-field, in order") {
  std::vector<InvSlot> inv;
  server::parseInvBlob("2002:1:1:4:73:2:3;3001:5:0:0:100:0:0;2001:1:0:1:10:1:1", inv);
  REQUIRE(inv.size() == 3);
  CHECK(inv[0].itemId == 2002);
  CHECK(inv[0].qty == 1);
  CHECK(inv[0].equipped);
  CHECK(inv[0].aura == 4);
  CHECK(inv[0].durability == 73);
  CHECK(inv[0].affix == 2);
  CHECK(inv[0].refine == 3);
  CHECK(inv[1].itemId == 3001);
  CHECK(inv[1].qty == 5);
  CHECK_FALSE(inv[1].equipped);
  CHECK(inv[1].aura == 0);
  CHECK(inv[1].durability == 100);
  CHECK(inv[1].affix == 0);
  CHECK(inv[1].refine == 0);
  CHECK(inv[2].itemId == 2001);
  CHECK(inv[2].durability == 10);
  CHECK(inv[2].affix == 1);
  CHECK(inv[2].refine == 1);
}

TEST_CASE("inv blob: dormant gear (durability 0) survives the parse") {
  // The launderer's worst sin: 0-durability (dormant) equipped gear came back
  // from replay at durability 100 — full stats from a broken item.
  std::vector<InvSlot> inv;
  server::parseInvBlob("2002:1:1:0:0:0:0;", inv);
  REQUIRE(inv.size() == 1);
  CHECK(inv[0].equipped);
  CHECK(inv[0].durability == 0);
}

TEST_CASE("inv blob: no stacking, no reordering — duplicate ids keep slots") {
  // debugGive would stack the two 2001 records / first-match patch them;
  // the login grammar must preserve exactly what the blob says.
  std::vector<InvSlot> inv;
  server::parseInvBlob("2001:1:1:2:50:0:1;2001:3:0:0:100:0:0", inv);
  REQUIRE(inv.size() == 2);
  CHECK(inv[0].qty == 1);
  CHECK(inv[0].equipped);
  CHECK(inv[0].durability == 50);
  CHECK(inv[0].refine == 1);
  CHECK(inv[1].qty == 3);
  CHECK_FALSE(inv[1].equipped);
  CHECK(inv[1].durability == 100);
}

TEST_CASE("inv blob: legacy short tails parse with InvSlot defaults") {
  std::vector<InvSlot> inv;
  server::parseInvBlob("2001:3:1;2002:1:0:2;2101:1:1:0:55;2003:2:0:1:80:3", inv);
  REQUIRE(inv.size() == 4);
  // v3 3-field: equipped only
  CHECK(inv[0].equipped);
  CHECK(inv[0].aura == 0);
  CHECK(inv[0].durability == 100);
  CHECK(inv[0].affix == 0);
  CHECK(inv[0].refine == 0);
  // v5 4-field: +aura
  CHECK(inv[1].aura == 2);
  CHECK(inv[1].durability == 100);
  // v8 5-field: +durability
  CHECK(inv[2].durability == 55);
  CHECK(inv[2].affix == 0);
  // v9 6-field: +affix
  CHECK(inv[3].affix == 3);
  CHECK(inv[3].refine == 0);
}

TEST_CASE("inv blob: malformed records are skipped, parsing continues") {
  std::vector<InvSlot> inv;
  server::parseInvBlob("junk;1:2;x:y:z;2001:4:1:0:100:0:0", inv);
  REQUIRE(inv.size() == 1);
  CHECK(inv[0].itemId == 2001);
  CHECK(inv[0].qty == 4);
}

TEST_CASE("inv blob: canonical serializer is a lossless round-trip") {
  std::vector<InvSlot> inv;
  server::parseInvBlob("2002:1:1:4:73:2:3;4002:32:0:0:100:0:0;2001:1:1:0:0:0:0", inv);
  const std::string canon = server::canonicalInvBlob(inv);
  CHECK(canon == "2002:1:1:4:73:2:3;4002:32:0:0:100:0:0;2001:1:1:0:0:0:0;");
  std::vector<InvSlot> again;
  server::parseInvBlob(canon, again);
  REQUIRE(again.size() == inv.size());
  for (size_t i = 0; i < inv.size(); ++i) {
    CHECK(again[i].itemId == inv[i].itemId);
    CHECK(again[i].qty == inv[i].qty);
    CHECK(again[i].equipped == inv[i].equipped);
    CHECK(again[i].aura == inv[i].aura);
    CHECK(again[i].durability == inv[i].durability);
    CHECK(again[i].affix == inv[i].affix);
    CHECK(again[i].refine == inv[i].refine);
  }
}

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
}  // namespace

TEST_CASE("inv blob: login parse feeds combat stats identically live/replay") {
  // End-to-end-flavour pin: an equipped, refined blade parsed from a blob
  // must drive equippedWeaponDmg exactly as a live-granted one does — i.e.
  // no field the combat lane reads may be dropped by the grammar.
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& e = w.spawn("blobby", 0, sim::TilePos{5, 5});
  server::parseInvBlob("2002:1:1:0:100:0:3", e.inv);  // blade, equipped, refine 3
  const std::uint32_t viaBlob = w.debugWeaponDmg(e);

  server::World w2;
  REQUIRE(w2.loadFrom(makeArena()));
  server::Entity& e2 = w2.spawn("blobby", 0, sim::TilePos{5, 5});
  REQUIRE(w2.debugGive(e2, 2002, 1));
  for (auto& sl : e2.inv)
    if (sl.itemId == 2002) { sl.equipped = true; sl.refine = 3; }
  CHECK(viaBlob == w2.debugWeaponDmg(e2));
  CHECK(viaBlob == 24);  // Pit Blade 18 + two refine steps (+2 dmg each)
}
