// T-105: equipped gear is not on the trade table — validator and swap must
// share ONE grammar. Pre-fix, tradeCommit's satisfiable() counted equipped
// stacks while deduce() skipped them, so an equipped-only offer passed
// validation and the swap executed with the item silently undelivered:
// offer your one equipped sword, take the partner's gold, keep the sword —
// and logs/trades.log recorded a completed swap (scam primitive, contradicts
// ADR-0011 commit-time validation).
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

std::string tmpLog() {
  return (std::filesystem::temp_directory_path() / "t105_trades.log").string();
}

std::string readAll(const std::string& path) {
  std::ifstream f(path);
  if (!f) return "";
  return std::string((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
}

server::Entity* spawnP(server::World& w, const char* name, int x, int y) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y});
  return w.find(e.id);
}

int countItem(const server::Entity& e, std::uint32_t itemId, bool equippedOnly) {
  int n = 0;
  for (const server::InvSlot& sl : e.inv)
    if (sl.itemId == itemId && sl.equipped == equippedOnly) n += sl.qty;
  return n;
}
}  // namespace

TEST_CASE("T-105: equipped-only offer cancels the commit; nothing moves") {
  const std::string path = tmpLog();
  std::remove(path.c_str());
  server::World::setTradeLogPath(path);
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "scammer", 10, 10);
  server::Entity* b = spawnP(w, "mark", 11, 10);
  REQUIRE(w.debugGive(*a, 2001, 1));  // one Rusty Shank...
  REQUIRE(w.toggleEquip(*a, 0));      // ...worn: the only copy
  REQUIRE(countItem(*a, 2001, /*equippedOnly=*/true) == 1);
  a->gold = 0;
  b->gold = 100;

  REQUIRE(w.tradeOpen(*a, b->id));
  w.tradeOffer(*a, 2001, 1);    // offers the sword he is wearing
  w.tradeOfferGold(*b, 100);    // mark pays up front
  w.tradeCommit(*a);
  w.tradeCommit(*b);            // handshake completes — pre-fix: swap "executed"

  // post-fix: cancelled as unsatisfiable, economy untouched both ways
  CHECK(a->tradeWith == 0);
  CHECK(b->tradeWith == 0);
  CHECK(countItem(*a, 2001, true) == 1);   // scammer still wears it
  CHECK(countItem(*b, 2001, false) == 0);  // mark got nothing
  CHECK(b->gold == 100u);                  // mark keeps his coin
  CHECK(a->gold == 0u);                    // no payment landed
  CHECK(readAll(path).empty());            // no audit line: nothing executed
  std::remove(path.c_str());
  server::World::setTradeLogPath("logs/trades.log");
}

TEST_CASE("T-105: offer exceeding the UNEQUIPPED count cancels (mixed stack)") {
  const std::string path = tmpLog();
  std::remove(path.c_str());
  server::World::setTradeLogPath(path);
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "halfkit", 10, 10);
  server::Entity* b = spawnP(w, "buyer", 11, 10);
  REQUIRE(w.debugGive(*a, 2001, 1));  // slot 0
  REQUIRE(w.debugGive(*a, 2001, 1));  // slot 1 (stackMax 1 => separate slots)
  REQUIRE(w.toggleEquip(*a, 0));      // wears one, carries one spare
  b->gold = 80;

  REQUIRE(w.tradeOpen(*a, b->id));
  w.tradeOffer(*a, 2001, 2);  // offers BOTH — only 1 is deliverable
  w.tradeOfferGold(*b, 80);
  w.tradeCommit(*a);
  w.tradeCommit(*b);

  CHECK(a->tradeWith == 0);
  CHECK(countItem(*a, 2001, true) == 1);   // worn blade untouched
  CHECK(countItem(*a, 2001, false) == 1);  // spare untouched: all-or-nothing
  CHECK(countItem(*b, 2001, false) == 0);
  CHECK(b->gold == 80u);
  CHECK(readAll(path).empty());
  std::remove(path.c_str());
  server::World::setTradeLogPath("logs/trades.log");
}

TEST_CASE("T-105: positive control — unequipped spare transfers, worn stays") {
  const std::string path = tmpLog();
  std::remove(path.c_str());
  server::World::setTradeLogPath(path);
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "seller", 10, 10);
  server::Entity* b = spawnP(w, "buyer", 11, 10);
  REQUIRE(w.debugGive(*a, 2001, 1));  // slot 0
  REQUIRE(w.debugGive(*a, 2001, 1));  // slot 1
  REQUIRE(w.toggleEquip(*a, 0));      // wears slot 0, spare in slot 1
  a->gold = 0;
  b->gold = 80;

  REQUIRE(w.tradeOpen(*a, b->id));
  w.tradeOffer(*a, 2001, 1);  // offers exactly the spare
  w.tradeOfferGold(*b, 80);
  w.tradeCommit(*a);
  w.tradeCommit(*b);

  CHECK(a->tradeWith == 0);
  CHECK(b->tradeWith == 0);
  CHECK(countItem(*a, 2001, true) == 1);   // still wearing his
  CHECK(countItem(*a, 2001, false) == 0);  // spare gone
  CHECK(countItem(*b, 2001, false) == 1);  // delivered
  CHECK(b->gold == 0u);
  CHECK(a->gold == 80u);
  const std::string contents = readAll(path);
  CHECK(contents.find("a_gives=(2001:1)+0g") != std::string::npos);
  std::remove(path.c_str());
  server::World::setTradeLogPath("logs/trades.log");
}
