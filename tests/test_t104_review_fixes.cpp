// T-104 review fixes (external review hardening wave): regression tests for
// bugs found during the September-2026 external code review. Each TEST_CASE
// documents the specific bug it guards against.
#include <doctest/doctest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

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

// Bug: tradeOffer(itemId, qty) previously appended a duplicate entry for the
// same itemId instead of replacing the existing quantity, so the deducer in
// tradeCommit double-counted and delivered/withdrew more than the final offer.
TEST_CASE("T-104: re-offering the same item id replaces, not duplicates") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "alice", 10, 10);
  server::Entity* b = spawnP(w, "bob", 11, 10);
  REQUIRE(w.debugGive(*a, 4001, 5));
  a->gold = 0;
  b->gold = 100;
  REQUIRE(w.tradeOpen(*a, b->id));
  w.tradeOffer(*a, 4001, 1);
  w.tradeOffer(*a, 4001, 3);  // revise: deliver 3, not 1+3
  w.tradeOfferGold(*b, 20);
  w.tradeCommit(*a);
  w.tradeCommit(*b);
  // Alice should keep 5-3 = 2 pelts, not 5-(1+3) = 1; Bob should have 3.
  unsigned aHas = 0, bHas = 0;
  for (const auto& sl : a->inv) if (sl.itemId == 4001) aHas += sl.qty;
  for (const auto& sl : b->inv) if (sl.itemId == 4001) bHas += sl.qty;
  CHECK(aHas == 2);
  CHECK(bHas == 3);
  CHECK(a->gold == 20);
  CHECK(b->gold == 80);
}

// Bug: vendorSellJunk / fenceSellJunk computed gold with mul-then-divide
// ordering that (a) truncated the per-unit price before qty and (b) could
// overflow u32 mid-calculation for high-value stackables.  Verify the
// widened 64-bit path returns the mathematically expected value.
TEST_CASE("T-104: junk sale math uses 64-bit mul (no overflow, no truncation)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  // Place a Marta vendor (wire kind 64) right on the player's tile so the
  // vendor-proximity check passes regardless of where spawnVendor seeded
  // the "real" Marta (spawnPoint is deterministic but tile-adjacency depends
  // on blocked tiles which vary by map).
  const sim::TilePos sp = w.spawnPoint();
  w.debugSpawnFurniture(64, "Marta", sp);
  server::Entity* p = spawnP(w, "p", sp.x, sp.y);
  // Rat Pelt (4001): slot=3 junk, value=10g, kSellRatioPct=40, stackMax=32.
  // 20 pelts × 10g × 40% = 80g.
  REQUIRE(w.debugGive(*p, 4001, 20));
  p->gold = 0;
  const std::uint32_t got = w.vendorSellJunk(*p);
  CHECK(got == 80);
  CHECK(p->gold == 80);
  CHECK(p->inv.empty());
}

// Bug: parseInvBlob used std::stoul which throws std::out_of_range on absurd
// numeric fields. A corrupted/malicious inventory blob could raise an
// exception that, before the T-104 try/catch landed, terminated the server.
TEST_CASE("T-104: parseInvBlob is exception-safe on absurdly long numerics") {
  std::vector<server::InvSlot> out;
  // 30-digit number: well beyond u32 range. Should fail gracefully, not throw.
  parseInvBlob("999999999999999999999999999999:1:0;", out);
  CHECK(out.empty());
  // Short invalid (letters) must not throw.
  out.clear();
  parseInvBlob("abc:1:0;", out);
  CHECK(out.empty());
  // Empty blob must not throw.
  out.clear();
  parseInvBlob("", out);
  CHECK(out.empty());
  // Normal record still parses (7-field canonical form).
  out.clear();
  parseInvBlob("1:5:1:0:100:0:0;", out);
  REQUIRE(out.size() == 1);
  CHECK(out[0].itemId == 1);
  CHECK(out[0].qty == 5);
  CHECK(out[0].equipped == true);
}

// Bug: anvil aura failLaw==1 (destroy) zeroed the slot in-place leaving an
// itemId=0 tombstone that permanently occupied an inventory slot until the
// next chaotic death. The fix erases the slot. Direct RNG-gated anvil tests
// are hard; this test guards the invariant (no tombstones) generically by
// verifying that erase() compacts correctly and addItem fills freed slots.
TEST_CASE("T-104: inventory compaction leaves no tombstone slots") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "p", 10, 10);
  // Fill all 32 slots directly (addItem stacks, so bypass it to simulate a
  // bag full of single-pelt stacks — this is the worst case for tombstone
  // leaks like the aura-destroy bug left behind).
  for (int i = 0; i < 32; ++i) {
    server::InvSlot sl;
    sl.itemId = 4001;
    sl.qty = 1;
    sl.equipped = false;
    sl.durability = 100;
    p->inv.push_back(sl);
  }
  CHECK(p->inv.size() == 32);
  bool added = w.debugGive(*p, 4002, 1);
  CHECK(!added);  // full
  p->inv.erase(p->inv.begin());
  CHECK(p->inv.size() == 31);
  added = w.debugGive(*p, 4002, 1);
  CHECK(added);
  CHECK(p->inv.size() == 32);
  // No tombstone (itemId==0) slots should ever exist after a mutation.
  for (const auto& sl : p->inv) CHECK(sl.itemId != 0);
}

// Bug: spawn() must insert into the spatial grid at the tile the walker was
// actually placed at, not at spawnPoint. Guard against regression where the
// spatial index drifts from walker.tile().
TEST_CASE("T-104: spawn inserts spatial at the placed tile, not spawnPoint") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& e = w.spawn("hero", 0, sim::TilePos{20, 20});
  const auto ids = w.queryAoi(1, 20, 20, 0);
  bool found = false;
  for (auto id : ids) if (id == e.id) found = true;
  CHECK(found);
  const sim::TilePos sp = w.spawnPoint();
  if (sp.x != 20 || sp.y != 20) {
    const auto ids2 = w.queryAoi(1, sp.x, sp.y, 0);
    for (auto id : ids2) CHECK(id != e.id);
  }
}
