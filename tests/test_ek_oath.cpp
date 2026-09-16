// T-130 EK ledger + L19 town oath (H4 night war, 2-3/3).
// Pins: oath gate matrix, EK credit matrix (enemy/same-town/neutral/
// chaotic-victim/duel), ekBoard ordering + readout, v11->v12 migration +
// saveProgress/loginOrCreate round-trip.
#include <doctest/doctest.h>

#include <cstdint>
#include <filesystem>
#include <string>

#include <sqlite3.h>

#include "command.h"
#include "content/towns.h"
#include "persist.h"
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

std::string tmpDbPath(const char* name) {
  return (std::filesystem::temp_directory_path() / name).string();
}

void rmDb(const std::string& p) {
  std::filesystem::remove(p);
  std::filesystem::remove(p + "-wal");
  std::filesystem::remove(p + "-shm");
}
}  // namespace

TEST_CASE("T-130: oath gate matrix") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "aspirant", 5, 5);
  p->level = 18;
  CHECK_FALSE(w.oath(*p, 1));  // under level
  CHECK(p->townId == 0u);
  p->level = 19;
  CHECK_FALSE(w.oath(*p, 0));  // unsworn is not a town
  CHECK_FALSE(w.oath(*p, 3));  // no third town
  REQUIRE(w.oath(*p, content::kTownThornwall));
  CHECK(p->townId == 1u);
  CHECK_FALSE(w.oath(*p, 2));  // sworn is sworn (no respec)
  CHECK(p->townId == 1u);
  bool crier = false;
  for (const auto& ev : w.events()) {
    if (ev.chatCh == 2 &&
        ev.chatText.find("swears to Thornwall") != std::string::npos)
      crier = true;
  }
  CHECK(crier);
  p->dead = true;
  server::Entity* q = spawnP(w, "ghost", 8, 8);
  q->level = 25;
  q->dead = true;
  CHECK_FALSE(w.oath(*q, 2));
}

TEST_CASE("T-130: journaled oath path") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "roundtrip", 5, 5);
  p->level = 19;
  server::Command c;
  c.kind = server::Command::kOath;
  c.a = 2;
  server::applyWorldCommand(w, *p, c);
  CHECK(p->townId == 2u);
}

TEST_CASE("T-130: war kills mint EK with no stain") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* killer = spawnP(w, "ashen", 5, 5);
  server::Entity* victim = spawnP(w, "synod", 6, 5);
  killer->level = 19;
  victim->level = 19;
  REQUIRE(w.oath(*killer, 1));
  REQUIRE(w.oath(*victim, 2));
  w.debugKillPlayerBy(*victim, killer);
  CHECK(killer->ek == 1u);
  CHECK(killer->karma == 0);  // war is not murder: no stain
  bool fame = false;
  for (const auto& ev : w.events()) {
    if (ev.chatCh == 2 &&
        ev.chatText.find("earns an enemy kill") != std::string::npos)
      fame = true;
  }
  CHECK(fame);
}

TEST_CASE("T-130: same-town and neutral kills still stain, mint nothing") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  // same town
  server::Entity* a = spawnP(w, "loyal", 5, 5);
  server::Entity* b = spawnP(w, "traitor", 6, 5);
  a->level = 19;
  b->level = 19;
  REQUIRE(w.oath(*a, 1));
  REQUIRE(w.oath(*b, 1));
  w.debugKillPlayerBy(*b, a);
  CHECK(a->ek == 0u);
  CHECK(a->karma < 0);
  // neutral killer vs sworn victim
  server::World w2;
  REQUIRE(w2.loadFrom(makeArena()));
  server::Entity* c = spawnP(w2, "neutral", 5, 5);
  server::Entity* d = spawnP(w2, "sworn", 6, 5);
  d->level = 19;
  REQUIRE(w2.oath(*d, 2));
  w2.debugKillPlayerBy(*d, c);
  CHECK(c->ek == 0u);
  CHECK(c->karma < 0);
}

TEST_CASE("T-130: chaotic enemy victims still grant EK; duels never do") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* killer = spawnP(w, "hunter", 5, 5);
  server::Entity* victim = spawnP(w, "redfoe", 6, 5);
  killer->level = 19;
  victim->level = 19;
  REQUIRE(w.oath(*killer, 1));
  REQUIRE(w.oath(*victim, 2));
  victim->karma = -100;  // red AND sworn to the enemy
  w.debugKillPlayerBy(*victim, killer);
  CHECK(killer->ek == 1u);
  // duel: consensual steel mints nothing
  server::World w2;
  REQUIRE(w2.loadFrom(makeArena()));
  server::Entity* d1 = spawnP(w2, "duelist1", 5, 5);
  server::Entity* d2 = spawnP(w2, "duelist2", 6, 5);
  d1->level = 19;
  d2->level = 19;
  REQUIRE(w2.oath(*d1, 1));
  REQUIRE(w2.oath(*d2, 2));
  d1->duelWith = d2->id;
  d2->duelWith = d1->id;
  d1->duelUntil = 100000;
  d2->duelUntil = 100000;
  w2.debugKillPlayerBy(*d2, d1);
  CHECK(d1->ek == 0u);
}

TEST_CASE("T-130: ekBoard orders desc, skips zero, ekReadout directs") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "third", 5, 5);
  server::Entity* b = spawnP(w, "first", 6, 5);
  server::Entity* cc = spawnP(w, "second", 7, 5);
  server::Entity* z = spawnP(w, "quiet", 8, 8);
  a->townId = 1;
  b->townId = 2;
  cc->townId = 1;
  z->townId = 2;
  a->ek = 1;
  b->ek = 3;
  cc->ek = 2;
  const auto rows = w.ekBoard(5);
  REQUIRE(rows.size() == 3u);  // zero-ek excluded
  CHECK(std::get<0>(rows[0]) == "first");
  CHECK(std::get<0>(rows[1]) == "second");
  CHECK(std::get<0>(rows[2]) == "third");
  CHECK(w.ekBoard(2).size() == 2u);  // cap holds
  // readout: directed 255-lines to the requester
  w.ekReadout(*z);
  int lines = 0;
  for (const auto& ev : w.events()) {
    if (ev.chatCh == 255 && ev.aboutId == z->id &&
        ev.chatText.rfind("EK ", 0) == 0)
      ++lines;
  }
  CHECK(lines == 3);
  server::World w2;
  REQUIRE(w2.loadFrom(makeArena()));
  server::Entity* lone = spawnP(w2, "lone", 5, 5);
  w2.ekReadout(*lone);
  bool none = false;
  for (const auto& ev : w2.events()) {
    if (ev.chatCh == 255 && ev.aboutId == lone->id &&
        ev.chatText.find("no enemy kills") != std::string::npos)
      none = true;
  }
  CHECK(none);
}

TEST_CASE("T-130: v11 -> v12 migration adds town/ek defaulting 0") {
  const std::string path = tmpDbPath("bh_oath_mig.bhdb");
  rmDb(path);
  sqlite3* raw = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &raw) == SQLITE_OK);
  REQUIRE(sqlite3_exec(raw,
                       "CREATE TABLE IF NOT EXISTS accounts (id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE COLLATE NOCASE, salt INTEGER NOT NULL, pwhash INTEGER NOT NULL, created INTEGER NOT NULL DEFAULT (strftime('%s','now')));"
                       "CREATE TABLE IF NOT EXISTS characters (id INTEGER PRIMARY KEY, account_id INTEGER NOT NULL REFERENCES accounts(id), name TEXT NOT NULL, map_id INTEGER NOT NULL DEFAULT 1, x INTEGER NOT NULL DEFAULT 0, y INTEGER NOT NULL DEFAULT 0, level INTEGER NOT NULL DEFAULT 1, xp INTEGER NOT NULL DEFAULT 0, created INTEGER NOT NULL DEFAULT (strftime('%s','now')), str INTEGER NOT NULL DEFAULT 8, vit INTEGER NOT NULL DEFAULT 8, dex INTEGER NOT NULL DEFAULT 8, stat_points INTEGER NOT NULL DEFAULT 0, gold INTEGER NOT NULL DEFAULT 50, inv TEXT NOT NULL DEFAULT '', anvil_mercy INTEGER NOT NULL DEFAULT 0, karma INTEGER NOT NULL DEFAULT 0, class_id INTEGER NOT NULL DEFAULT 1, sword_skill INTEGER NOT NULL DEFAULT 0, swing_lands INTEGER NOT NULL DEFAULT 0);"
                       "INSERT INTO accounts(id, name, salt, pwhash) VALUES(1, 'veteran', 123, 456);"
                       "INSERT INTO characters(id, account_id, name, level, xp) VALUES(1, 1, 'veteran', 19, 100);"
                       "PRAGMA user_version=11;",
                       nullptr, nullptr, nullptr) == SQLITE_OK);
  sqlite3_close(raw);

  server::Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  sqlite3* check = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &check) == SQLITE_OK);
  sqlite3_stmt* st = nullptr;
  REQUIRE(sqlite3_prepare_v2(check, "PRAGMA user_version;", -1, &st, nullptr) ==
          SQLITE_OK);
  REQUIRE(sqlite3_step(st) == SQLITE_ROW);
  CHECK(sqlite3_column_int(st, 0) == 16);  // T-153: schema v16 (was v15)
  sqlite3_finalize(st);
  REQUIRE(sqlite3_prepare_v2(check,
                             "SELECT town_id, ek FROM characters WHERE id=1;",
                             -1, &st, nullptr) == SQLITE_OK);
  REQUIRE(sqlite3_step(st) == SQLITE_ROW);
  CHECK(sqlite3_column_int(st, 0) == 0);
  CHECK(sqlite3_column_int(st, 1) == 0);
  sqlite3_finalize(st);
  sqlite3_close(check);
  rmDb(path);
}

TEST_CASE("T-130: saveProgress + loginOrCreate round-trips town and EK") {
  const std::string path = tmpDbPath("bh_oath_roundtrip.bhdb");
  rmDb(path);
  server::Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  server::CharacterRow row;
  std::uint8_t reason = 0;
  REQUIRE(db.loginOrCreate("oathkeeper", "pw123", &row, &reason, &err));
  CHECK(row.townId == 0);
  CHECK(row.ek == 0);
  db.saveProgress(row.id, 19, row.xp, row.str, row.vit, row.dex,
                  row.statPoints, row.gold, row.invBlob, row.anvilMercy,
                  row.karma, row.classId, row.swordSkill, row.swingLands, 2, 7,
                  row.pledgeId, row.pledgeRank, row.sex, row.lastDeathTick,
                  row.lastDebtXp, row.lastResTick, row.bountyMob, row.bountyCycle);
  server::Db db2;
  REQUIRE(db2.open(path, &err));
  server::CharacterRow row2;
  REQUIRE(db2.loginOrCreate("oathkeeper", "pw123", &row2, &reason, &err));
  CHECK(row2.townId == 2);
  CHECK(row2.ek == 7);
  CHECK(row2.level == 19);
  rmDb(path);
}
