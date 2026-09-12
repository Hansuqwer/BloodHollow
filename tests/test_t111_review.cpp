// T-111: critical-review fixes (2026-09-12).
// Pins three classes the hardening wave (T-104..110) did not close:
//   F1 — cross-zone respawn: spatial grid must match walker at home_/gallows
//   F2 — tryAnvil: weapon slot survives part-stack erases (no dangling InvSlot*)
//   F3 — parseInvBlob: throw-free int parse (oversized digit strings reject)
#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <vector>

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

sim::Map makeCrypt() {
  sim::Map m;
  m.w = 12;
  m.h = 12;
  m.tileW = 64;
  m.tileH = 32;
  m.ground.assign(12 * 12, 0);
  m.zone.assign(12 * 12, 0);
  m.blocked.assign(12 * 12, 0);
  return m;
}

bool aoiContains(const server::World& w, std::uint16_t zone, int x, int y,
                 std::uint32_t id) {
  const auto ids = w.queryAoi(zone, x, y, /*radiusTiles=*/0);
  for (const std::uint32_t q : ids)
    if (q == id) return true;
  return false;
}
}  // namespace

TEST_CASE("T-111 F1: cross-zone gallows respawn keeps spatial == walker") {
  // Town with a portal into a crypt; die wanted in the crypt; crawl back.
  // Pre-fix: walker at gallows, spatial at spawnPoint → AoI at the body
  // missed the player until the first step.
  sim::Map town = makeArena();
  sim::PortalDef fd;
  fd.x = 20;
  fd.y = 26;
  fd.w = 1;
  fd.h = 1;
  fd.targetMapId = 3;
  fd.targetX = 4;
  fd.targetY = 4;
  town.portals.push_back(fd);

  server::World w;
  REQUIRE(w.loadFrom(town));
  REQUIRE(w.loadZoneFrom(3, makeCrypt()));

  server::Entity& raw = w.spawn("exile", 0, sim::TilePos{20, 25});
  const std::uint32_t pid = raw.id;
  w.queuePath(*w.find(pid), {20, 26});
  for (int i = 0; i < 120 && w.find(pid)->zoneId != 3; ++i) w.tick();
  REQUIRE(w.find(pid)->zoneId == 3);

  // Wanted mark forces gallows bind (clean karma still).
  w.find(pid)->wantedUntil = w.tickCount() + 100000;
  w.debugKillPlayer(*w.find(pid));
  REQUIRE(w.find(pid)->dead);

  for (int i = 0; i < 70; ++i) w.tick();
  const server::Entity* p = w.find(pid);
  REQUIRE(p != nullptr);
  CHECK_FALSE(p->dead);
  CHECK(p->zoneId == 1);

  const sim::TilePos body = p->walker.tile();
  const sim::TilePos gallows = w.gallowsTile(1);
  const sim::TilePos bind = w.spawnPoint();
  CHECK(body == gallows);  // wanted → gallows fiction
  // Spatial must answer at the body, not (only) at the bindstone.
  CHECK(aoiContains(w, 1, body.x, body.y, pid));
  if (bind != body) {
    // Pre-fix failure mode: AoI at spawnPoint still saw the corpse-less id.
    // After the fix the id lives only at body; bind may or may not overlap
    // the same cell depending on gallows search — only assert mismatch case.
    CHECK_FALSE(aoiContains(w, 1, bind.x, bind.y, pid));
  }
}

TEST_CASE("T-111 F1: cross-zone lawful respawn spatial == bindstone") {
  sim::Map town = makeArena();
  sim::PortalDef fd;
  fd.x = 20;
  fd.y = 26;
  fd.w = 1;
  fd.h = 1;
  fd.targetMapId = 3;
  fd.targetX = 4;
  fd.targetY = 4;
  town.portals.push_back(fd);

  server::World w;
  REQUIRE(w.loadFrom(town));
  REQUIRE(w.loadZoneFrom(3, makeCrypt()));

  const std::uint32_t pid = w.spawn("pilgrim", 0, sim::TilePos{20, 25}).id;
  w.queuePath(*w.find(pid), {20, 26});
  for (int i = 0; i < 120 && w.find(pid)->zoneId != 3; ++i) w.tick();
  REQUIRE(w.find(pid)->zoneId == 3);

  w.debugKillPlayer(*w.find(pid));
  for (int i = 0; i < 70; ++i) w.tick();
  const server::Entity* p = w.find(pid);
  REQUIRE(p != nullptr);
  CHECK(p->zoneId == 1);
  const sim::TilePos body = p->walker.tile();
  CHECK(body == w.spawnPoint());
  CHECK(aoiContains(w, 1, body.x, body.y, pid));
}

TEST_CASE("T-111 F2: tryAnvil blesses the weapon after part-stack erase") {
  // Layout: [pelt stack @0][shank equipped @1]. Tier-I toll eats 30 pelts
  // and erases slot 0 — pre-fix wslot* still pointed at the old slot-1
  // address (now the shank slid into 0, or a freed neighbour under capacity
  // growth). Post-fix: aura lands on the shank by tracked index.
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  bool anvilPresent = false;
  for (const auto& e : w.entities())
    if (e.wireKind == 65) anvilPresent = true;
  REQUIRE(anvilPresent);

  const std::uint32_t pid = w.spawn("smith", 0, std::nullopt).id;
  server::Entity* p = w.find(pid);
  // Give pelts FIRST so they occupy slot 0; shank second → slot 1.
  REQUIRE(w.debugGive(*p, 4001, 30));  // exact toll — stack empties + erases
  REQUIRE(w.debugGive(*p, 2001, 1));   // Rusty Shank
  REQUIRE(p->inv.size() >= 2);
  REQUIRE(p->inv[0].itemId == 4001);
  REQUIRE(p->inv[1].itemId == 2001);
  REQUIRE(w.toggleEquip(*p, 1));  // equip shank at index 1
  REQUIRE(p->inv[1].equipped);
  p->gold = 120;
  p->swordSkill = 25;

  REQUIRE(w.tryAnvil(*p, 1));
  server::Entity* q = w.find(pid);
  REQUIRE(q != nullptr);
  // Pelts gone; shank remains, now aura tier 1.
  bool sawShank = false;
  for (const auto& sl : q->inv) {
    if (sl.itemId == 2001) {
      sawShank = true;
      CHECK(sl.equipped);
      CHECK(sl.aura == 1);
    }
    CHECK(sl.itemId != 4001);  // toll consumed
  }
  CHECK(sawShank);
  CHECK(q->gold == 0);
  CHECK(w.debugWeaponDmg(*q) == 12u + 3u);  // Edge Rite flat
}

TEST_CASE("T-111 F3: parseInvBlob rejects oversized digit fields without throw") {
  // Pre-fix: digit-class check then std::stoul → out_of_range on 20 nines.
  std::vector<server::InvSlot> inv;
  // Must not throw; record is skipped (nf fields parse fail → continue).
  CHECK_NOTHROW(server::parseInvBlob(
      "99999999999999999999:1:0;2001:1:1:0:100:0:0;", inv));
  // Only the well-formed second record survives.
  REQUIRE(inv.size() == 1);
  CHECK(inv[0].itemId == 2001);
  CHECK(inv[0].equipped);

  inv.clear();
  CHECK_NOTHROW(server::parseInvBlob("2001:999999999999:1;", inv));
  CHECK(inv.empty());  // qty overflow → whole record dropped

  inv.clear();
  // Still accepts in-domain values (regression vs the throw-free rewrite).
  CHECK_NOTHROW(server::parseInvBlob("2002:1:1:4:73:2:3;", inv));
  REQUIRE(inv.size() == 1);
  CHECK(inv[0].itemId == 2002);
  CHECK(inv[0].durability == 73);
  CHECK(inv[0].refine == 3);
}
