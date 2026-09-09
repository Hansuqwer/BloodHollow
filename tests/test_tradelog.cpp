// T-080 trade transaction log: one audit line per EXECUTED swap, nothing
// for cancelled or unsatisfiable commits. Pinned line shape (tick, ids,
// names, offers, gold both ways). The path override keeps the suite off
// the real logs/trades.log; replay appends byte-identical lines (same
// tick/ids/offers — ops dedups trivially), so no replay test here.
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
  return (std::filesystem::temp_directory_path() / "t080_trades.log").string();
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
}  // namespace

TEST_CASE("T-080: executed swap appends exactly one pinned line") {
  const std::string path = tmpLog();
  std::remove(path.c_str());
  server::World::setTradeLogPath(path);
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "giver", 10, 10);
  server::Entity* b = spawnP(w, "taker", 11, 10);
  REQUIRE(w.debugGive(*a, 4001, 2));  // 2x Rat Pelt
  a->gold = 100;
  b->gold = 50;
  REQUIRE(w.tradeOpen(*a, b->id));
  w.tradeOffer(*a, 4001, 2);
  w.tradeOfferGold(*a, 10);
  w.tradeOfferGold(*b, 5);  // taker brings coin only
  w.tradeCommit(*a);
  w.tradeCommit(*b);  // handshake completes on the second commit
  const std::string log = readAll(path);
  CHECK(log ==
        "tick=0 a=" + std::to_string(a->id) + ":giver b=" + std::to_string(b->id) +
            ":taker a_gives=(4001:2)+10g b_gives=()+5g\n");
  std::remove(path.c_str());
  server::World::setTradeLogPath("logs/trades.log");
}

TEST_CASE("T-080: oversell and cancel append nothing") {
  const std::string path = tmpLog();
  std::remove(path.c_str());
  server::World::setTradeLogPath(path);
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "poor", 10, 10);
  server::Entity* b = spawnP(w, "mark", 11, 10);
  REQUIRE(w.debugGive(*a, 4001, 1));  // owns 1 pelt...
  REQUIRE(w.tradeOpen(*a, b->id));
  w.tradeOffer(*a, 4001, 5);  // ...offers 5: unsatisfiable
  w.tradeCommit(*a);
  w.tradeCommit(*b);
  CHECK(readAll(path).empty());
  // fresh window, then cancel: still nothing
  REQUIRE(w.tradeOpen(*a, b->id));
  w.tradeOffer(*a, 4001, 1);
  w.tradeCancel(*a, "cold feet");
  CHECK(readAll(path).empty());
  std::remove(path.c_str());
  server::World::setTradeLogPath("logs/trades.log");
}
