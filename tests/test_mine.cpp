// H2 mining: ore nodes in Bonehowl Mine (zone 4), /mine command, ore→anvil
// toll substitution (1 ore == 5 pelts).  Pins: refuse far / no-pick / dead;
// yield adds ore; anvil accepts 6 ore + skill 20 + 120g → aura tier 1.
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
#include "content/items.h"
#include "content/wirekind.h"
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

server::Entity* spawnP(server::World& w, const char* name, int x, int y,
                       std::uint16_t zoneId = 1) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y}, zoneId);
  return w.find(e.id);
}

void givePick(server::World& w, server::Entity& e) {
  w.debugGive(e, 2003, 1);
  // equip it
  for (auto& sl : e.inv) {
    if (sl.itemId == 2003) { sl.equipped = true; break; }
  }
}
}  // namespace

TEST_CASE("H2: refuse mine when far from node") {
  server::World w;
  sim::Map mine = makeArena();
  REQUIRE(w.loadZoneFrom(4, std::move(mine)));
  server::Entity* p = spawnP(w, "digger", 0, 0, 4);
  givePick(w, *p);
  CHECK_FALSE(w.tryMine(*p));
}

TEST_CASE("H2: refuse mine when dead") {
  server::World w;
  sim::Map mine = makeArena();
  REQUIRE(w.loadZoneFrom(4, std::move(mine)));
  // spawn near ore node (position 20,10 from spawnOreNodes)
  server::Entity* p = spawnP(w, "digger", 20, 11, 4);
  p->dead = true;
  givePick(w, *p);
  CHECK_FALSE(w.tryMine(*p));
}

TEST_CASE("H2: refuse mine without pick") {
  server::World w;
  sim::Map mine = makeArena();
  REQUIRE(w.loadZoneFrom(4, std::move(mine)));
  server::Entity* p = spawnP(w, "digger", 20, 11, 4);
  // no pick
  CHECK_FALSE(w.tryMine(*p));
}

TEST_CASE("H2: mine yields 1-2 Blackiron Ore") {
  server::World w;
  sim::Map mine = makeArena();
  REQUIRE(w.loadZoneFrom(4, std::move(mine)));
  server::Entity* p = spawnP(w, "digger", 20, 11, 4);
  givePick(w, *p);
  REQUIRE(w.tryMine(*p));
  // check inventory has ore
  std::uint32_t oreQty = 0;
  for (const auto& sl : p->inv) {
    if (sl.itemId == 5001) oreQty += sl.qty;
  }
  CHECK(oreQty >= 1);
  CHECK(oreQty <= 2);
  // event emitted
  bool mineLine = false;
  for (const auto& ev : w.events())
    if (ev.chatText.find("Blackiron Ore") != std::string::npos) mineLine = true;
  CHECK(mineLine);
}

TEST_CASE("H2: ore node exists in zone 4") {
  server::World w;
  sim::Map mine = makeArena();
  REQUIRE(w.loadZoneFrom(4, std::move(mine)));
  bool found = false;
  for (const auto& e : w.entities()) {
    if (e.zoneId == 4 && e.wireKind == content::kWireKindOreNode) found = true;
  }
  CHECK(found);
}

TEST_CASE("H2: anvil accepts 6 ore + skill 20 + 120g for tier 1") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  // spawn at zone 1 spawn point (near anvil)
  server::Entity& ref = w.spawn("smith", 0, std::nullopt);
  server::Entity* p = w.find(ref.id);
  // give weapon + swordSkill 20 + 6 ore + 120g
  w.debugGive(*p, 2001, 1);  // Rusty Shank
  for (auto& sl : p->inv) {
    if (sl.itemId == 2001) { sl.equipped = true; break; }
  }
  p->swordSkill = 20;
  w.debugGive(*p, 5001, 6);  // 6 Blackiron Ore
  p->gold = 200;
  REQUIRE(w.tryAnvil(*p, 1));
  // weapon should have aura 1
  for (const auto& sl : p->inv) {
    if (sl.itemId == 2001 && sl.equipped) {
      CHECK(sl.aura == 1);
    }
  }
  CHECK(p->gold == 80);  // 200 - 120
}

TEST_CASE("H2: tryMine through applyWorldCommand (replay path)") {
  server::World w;
  sim::Map mine = makeArena();
  REQUIRE(w.loadZoneFrom(4, std::move(mine)));
  server::Entity* p = spawnP(w, "digger", 20, 11, 4);
  givePick(w, *p);
  server::Command c;
  c.kind = server::Command::kMine;
  server::applyWorldCommand(w, *p, c);
  std::uint32_t oreQty = 0;
  for (const auto& sl : p->inv) {
    if (sl.itemId == 5001) oreQty += sl.qty;
  }
  CHECK(oreQty >= 1);
}
