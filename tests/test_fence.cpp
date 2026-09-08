// T-069 Smugglers' Cove fence: the no-questions lane Marta refuses.
// Era pins: fence pawns junk at 60% (Marta 40%) to ANY karma band; the secret
// stock (potion + rare junk) shows itself to chaotic eyes only at a 25%
// markup; trading with the fence never moves karma; Marta's T-056 refusals
// stand untouched. Lane routing lives in applyWorldCommand (kBuy/kSellJunk),
// so live and replay both take the same fence-or-Marta branch.
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
#include "content/items.h"
#include "sim/combat.h"
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

TEST_CASE("T-069: the fence pawns junk at 60% for anyone — red or clean") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  (void)w.debugSpawnFence(sim::TilePos{10, 10});

  server::Entity* red = spawnP(w, "redhand", 11, 10);
  red->karma = -10;
  REQUIRE(w.debugGive(*red, 4001, 3));  // 3x Rat Pelt, value 10
  const std::int32_t karmaBefore = red->karma;
  CHECK(w.nearFence(*red));
  CHECK(w.fenceSellJunk(*red) == 18u);  // 3 x 10 x 60% = 18
  CHECK(red->gold == 50u + 18u);
  CHECK(red->inv.empty());
  CHECK(red->karma == karmaBefore);  // no questions asked, no karma moved

  // the same lane pays a lawful courier: no gate on the pawn side
  server::Entity* law = spawnP(w, "courier", 10, 11);
  REQUIRE(w.debugGive(*law, 4002, 2));  // Ghoul Finger value 22 -> 13 each
  CHECK(w.fenceSellJunk(*law) == 26u);
  CHECK(law->gold == 50u + 26u);
  CHECK(law->karma == 0);  // neutral stays neutral
}

TEST_CASE("T-069: the crate shows itself to red eyes only (25% markup)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  (void)w.debugSpawnFence(sim::TilePos{10, 10});

  server::Entity* red = spawnP(w, "buyer", 11, 10);
  red->karma = -10;
  red->gold = 100;
  // Smuggled Vial value 45 -> 45*125% = 56 (integer math)
  REQUIRE(w.fenceBuy(*red, 3002, 1));
  CHECK(red->gold == 100u - 56u);
  CHECK(red->karma == -10);  // buying changed nothing

  // broke at the markup: refused atomically
  red->gold = 10;
  CHECK_FALSE(w.fenceBuy(*red, 4005, 1));  // Revenant Ash 75 -> 93

  // the clean-handed get the fiction line and nothing else
  server::Entity* clean = spawnP(w, "monk", 10, 11);
  clean->gold = 500;
  CHECK_FALSE(w.fenceBuy(*clean, 3002, 1));
  CHECK(clean->gold == 500u);
  CHECK(clean->inv.empty());
}

TEST_CASE("T-069: Sable sells exactly one crate; gear is not her trade") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  (void)w.debugSpawnFence(sim::TilePos{10, 10});
  server::Entity* red = spawnP(w, "redbuyer", 11, 10);
  red->karma = -10;
  red->gold = 1000;
  CHECK_FALSE(w.fenceBuy(*red, 2002, 1));  // Pit Blade: Marta's stock, not hers
  CHECK_FALSE(w.fenceBuy(*red, 2102, 1));  // Bone Plate likewise
  CHECK(red->gold == 1000u);
}

TEST_CASE("T-069: lane routing through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  (void)w.debugSpawnFence(sim::TilePos{10, 10});
  server::Entity* red = spawnP(w, "redcoin", 11, 10);
  red->karma = -10;
  REQUIRE(w.debugGive(*red, 4001, 2));  // 2 x 10 x 60% = 12

  server::Command sell;
  sell.kind = server::Command::kSellJunk;
  server::applyWorldCommand(w, *red, sell);
  CHECK(red->gold == 50u + 12u);
  CHECK(red->inv.empty());

  server::Command buy;
  buy.kind = server::Command::kBuy;
  buy.a = static_cast<std::int32_t>(3002);
  buy.b = 1;  // Smuggled Vial at 125% = 56g
  server::applyWorldCommand(w, *red, buy);
  CHECK(red->gold == 62u - 56u);
}

TEST_CASE("T-069: one Marta — the spawn spiral keeps its break") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  std::size_t vendors = 0;
  for (const auto& e : w.entities())
    if (e.wireKind == content::kWireKindVendor) ++vendors;
  CHECK(vendors == 1u);  // ~425 duplicate Martas shipped before this fix

  // Marta's lanes still work for a lawful shopper at her stall
  server::Entity* vend = nullptr;
  for (const auto& e : w.entities())
    if (e.wireKind == content::kWireKindVendor) {
      vend = w.find(e.id);
      break;
    }
  REQUIRE(vend != nullptr);
  server::Entity* p = spawnP(w, "shopper", vend->walker.tile().x + 1, vend->walker.tile().y);
  p->gold = 100;
  CHECK(w.vendorBuy(*p, 2001, 1));  // Rusty Shank 80g
  CHECK(p->gold == 20u);
}

TEST_CASE("T-069: Marta's refusals stand next door (T-056 regression)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* vend = nullptr;
  for (const auto& e : w.entities()) {
    if (e.wireKind == content::kWireKindVendor) {
      vend = w.find(e.id);
      break;
    }
  }
  REQUIRE(vend != nullptr);
  server::Entity* red = spawnP(w, "redpawn", vend->walker.tile().x + 1, vend->walker.tile().y);
  red->karma = -10;
  REQUIRE(w.debugGive(*red, 4001, 3));
  CHECK_FALSE(w.vendorBuy(*red, 3001, 1));  // red coin refused (karma gate first)
  CHECK(w.vendorSellJunk(*red) == 0u);      // pawn lane refused too
  CHECK(red->gold == 50u);                  // nothing moved
}
TEST_CASE("T-069: the town seeds exactly one Sable at the gallows pit") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  std::size_t fences = 0;
  const server::Entity* sable = nullptr;
  for (const auto& e : w.entities())
    if (e.wireKind == content::kWireKindFence) {
      ++fences;
      sable = &e;
    }
  REQUIRE(fences == 1u);
  CHECK(sable->name == "Sable the Fence");
  // within the r<4 gallows spiral, on a walkable tile
  bool nearGallows = false;
  for (int dy = -4; dy <= 4 && !nearGallows; ++dy)
    for (int dx = -4; dx <= 4 && !nearGallows; ++dx)
      if (std::max(std::abs(dx), std::abs(dy)) <= 4) {
        const sim::TilePos gp = w.gallowsTile(1);
        if (sable->walker.tile().x == gp.x + dx && sable->walker.tile().y == gp.y + dy)
          nearGallows = true;
      }
  CHECK(nearGallows);
  // a shopper parked on her tile is served; Marta's stall is elsewhere
  server::Entity* red = spawnP(w, "redseed", sable->walker.tile().x, sable->walker.tile().y);
  red->karma = -5;
  CHECK(w.nearFence(*red));
}
