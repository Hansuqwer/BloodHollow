// T-107: worldHash must see the economy/progression layer. The old oracle
// mixed only (id, zoneId, x, y, hp) per entity — inventory, gold, XP, level,
// karma, buffs, duel/party/trade state were all invisible to the per-tick
// replay gate. Project history proves the cost: the relog launderer
// (devlog 0023 / T-049x) corrupted inventory and equipped flags on login
// and the hash gate passed it; only separate BH_DUMP_ENTS probes caught it.
// These tests pin the widened hash: every economy/progression mutation must
// move the hash, and identical worlds must hash identically.
#include <doctest/doctest.h>

#include <cstdint>

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

int slotOf(const server::Entity& e, std::uint32_t itemId) {
  for (size_t i = 0; i < e.inv.size(); ++i)
    if (e.inv[i].itemId == itemId) return static_cast<int>(i);
  return -1;
}
}  // namespace

TEST_CASE("T-107: worldHash sees economy/progression mutations") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "Ledger", 5, 5);
  REQUIRE(p != nullptr);
  const std::uint64_t h0 = w.worldHash();

  SUBCASE("gold moves the hash") { p->gold += 5; }
  SUBCASE("an inventory item moves the hash") {
    REQUIRE(w.debugGive(*p, 2001, 1));
  }
  SUBCASE("the equipped flag moves the hash") {
    REQUIRE(w.debugGive(*p, 2001, 1));
    const int si = slotOf(*p, 2001);
    REQUIRE(si >= 0);
    REQUIRE(w.toggleEquip(*p, static_cast<std::uint8_t>(si)));
  }
  SUBCASE("xp moves the hash") { p->xp += 17; }
  SUBCASE("level + stat points move the hash") {
    p->level += 1;
    p->statPoints += 1;
  }
  SUBCASE("karma moves the hash") { w.bumpKarma(*p, 3); }
  SUBCASE("sword skill moves the hash") { p->swordSkill += 1; }
  SUBCASE("anvil mercy mask moves the hash") { p->anvilMercyMask |= 1u; }
  SUBCASE("a buff stamp moves the hash") { p->blessUntil = 12345; }
  SUBCASE("a curse stamp moves the hash") { p->curseUntil = 999; }
  SUBCASE("carried light moves the hash") {
    p->lightRadius = 4;
    p->lightUntil = 5000;
  }
  SUBCASE("party affiliation moves the hash") { p->partyId = 4242; }
  SUBCASE("duel state moves the hash") {
    p->duelWith = 77;
    p->duelUntil = 400;
  }
  SUBCASE("a trade gold offer moves the hash") { p->tradeOfferGold = 10; }
  SUBCASE("a trade item offer moves the hash") {
    p->tradeOfferItems.push_back({2001, 1});
  }
  SUBCASE("wanted status moves the hash") { p->wantedUntil = 300; }
  SUBCASE("mana moves the hash") { p->mp += 5; }
  SUBCASE("position still moves the hash (pre-T-107 coverage kept)") {
    p->walker.place(sim::TilePos{6, 6});
  }
  SUBCASE("hp still moves the hash (pre-T-107 coverage kept)") { p->hp -= 1; }

  CHECK(w.worldHash() != h0);
}

TEST_CASE("T-107: worldHash is deterministic for identical worlds") {
  auto build = [] {
    server::World w;
    w.loadFrom(makeArena());
    server::Entity* p = spawnP(w, "Twins", 5, 5);
    (void)w.debugGive(*p, 2001, 2);
    p->gold = 123;
    p->xp = 456;
    p->karma = -7;
    p->blessUntil = 4242;
    for (int i = 0; i < 25; ++i) w.tick();
    return w.worldHash();
  };
  CHECK(build() == build());

  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const std::uint64_t h = w.worldHash();
  CHECK(w.worldHash() == h);  // stable across calls, no hidden counters
}
