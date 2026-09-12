// T-118 B1: weapon-skill persistence — schema v11 additive-only, journal epoch 21.
// Verifies Db migration, saveProgress/load, and backward compat.

#include <filesystem>
#include <cstdio>
#include <string>

#include <doctest/doctest.h>
#include <sqlite3.h>

#include "persist.h"

using namespace bh::server;

namespace {

std::string tmpDbPath(const char* name) {
  return (std::filesystem::temp_directory_path() / name).string();
}

void rmDb(const std::string& p) {
  std::filesystem::remove(p);
  std::filesystem::remove(p + "-wal");
  std::filesystem::remove(p + "-shm");
}

} // namespace

TEST_CASE("weapon skill persist: v10 -> v11 migration adds columns with defaults 0") {
  const std::string path = tmpDbPath("bh_skill_mig.bhdb");
  rmDb(path);

  // Create a v10 DB manually (no sword_skill columns, user_version=10)
  sqlite3* raw = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &raw) == SQLITE_OK);
  char* msg = nullptr;
  REQUIRE(sqlite3_exec(raw,
    "CREATE TABLE IF NOT EXISTS accounts ("
    "  id INTEGER PRIMARY KEY,"
    "  name TEXT NOT NULL UNIQUE COLLATE NOCASE,"
    "  salt INTEGER NOT NULL,"
    "  pwhash INTEGER NOT NULL,"
    "  created INTEGER NOT NULL DEFAULT (strftime('%s','now'))"
    ");"
    "CREATE TABLE IF NOT EXISTS characters ("
    "  id INTEGER PRIMARY KEY,"
    "  account_id INTEGER NOT NULL REFERENCES accounts(id),"
    "  name TEXT NOT NULL,"
    "  map_id INTEGER NOT NULL DEFAULT 1,"
    "  x INTEGER NOT NULL DEFAULT 0,"
    "  y INTEGER NOT NULL DEFAULT 0,"
    "  level INTEGER NOT NULL DEFAULT 1,"
    "  xp INTEGER NOT NULL DEFAULT 0,"
    "  created INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
    "  str INTEGER NOT NULL DEFAULT 8,"
    "  vit INTEGER NOT NULL DEFAULT 8,"
    "  dex INTEGER NOT NULL DEFAULT 8,"
    "  stat_points INTEGER NOT NULL DEFAULT 0,"
    "  gold INTEGER NOT NULL DEFAULT 50,"
    "  inv TEXT NOT NULL DEFAULT '',"
    "  anvil_mercy INTEGER NOT NULL DEFAULT 0,"
    "  karma INTEGER NOT NULL DEFAULT 0,"
    "  class_id INTEGER NOT NULL DEFAULT 1"
    ");"
    "PRAGMA user_version=10;",
    nullptr, nullptr, &msg) == SQLITE_OK);
  sqlite3_close(raw);

  // Open via Db::open — should migrate to v11
  Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  // Check user_version now 11 and columns exist
  sqlite3* check = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &check) == SQLITE_OK);
  sqlite3_stmt* st = nullptr;
  REQUIRE(sqlite3_prepare_v2(check, "PRAGMA user_version;", -1, &st, nullptr) == SQLITE_OK);
  REQUIRE(sqlite3_step(st) == SQLITE_ROW);
  int uv = sqlite3_column_int(st, 0);
  sqlite3_finalize(st);
  // T-122: Db::open migrates to the CURRENT schema head (v12+); this test
  // only pins that the v10 -> v11 weapon-skill step ran (columns + defaults).
  CHECK(uv >= 11);

  bool hasSword = false, hasLands = false;
  REQUIRE(sqlite3_prepare_v2(check, "PRAGMA table_info(characters);", -1, &st, nullptr) == SQLITE_OK);
  while (sqlite3_step(st) == SQLITE_ROW) {
    const char* cn = reinterpret_cast<const char*>(sqlite3_column_text(st, 1));
    if (cn) {
      if (std::string(cn) == "sword_skill") hasSword = true;
      if (std::string(cn) == "swing_lands") hasLands = true;
    }
  }
  sqlite3_finalize(st);
  sqlite3_close(check);
  CHECK(hasSword);
  CHECK(hasLands);

  rmDb(path);
}

TEST_CASE("weapon skill persist: saveProgress + loginOrCreate round-trips skill") {
  const std::string path = tmpDbPath("bh_skill_roundtrip.bhdb");
  rmDb(path);

  Db db;
  std::string err;
  REQUIRE(db.open(path, &err));

  CharacterRow row;
  std::uint8_t reason = 0;
  REQUIRE(db.loginOrCreate("skilltester", "pw123", &row, &reason, &err));
  CHECK(row.swordSkill == 0);
  CHECK(row.swingLands == 0);

  // Simulate gaining skill: 20 skill, 500 lands
  db.saveProgress(row.id, row.level, row.xp, row.str, row.vit, row.dex,
                  row.statPoints, row.gold, row.invBlob, row.anvilMercy,
                  row.karma, row.classId, 20, 500,
                  row.pledgeId, row.pledgeRank);  // T-122: v12 columns

  // Reload via new Db instance
  Db db2;
  REQUIRE(db2.open(path, &err));
  CharacterRow row2;
  REQUIRE(db2.loginOrCreate("skilltester", "pw123", &row2, &reason, &err));
  CHECK(row2.swordSkill == 20);
  CHECK(row2.swingLands == 500);

  // Update again: 25 skill, 625 lands
  db2.saveProgress(row2.id, row2.level, row2.xp, row2.str, row2.vit, row2.dex,
                   row2.statPoints, row2.gold, row2.invBlob, row2.anvilMercy,
                   row2.karma, row2.classId, 25, 625,
                   row2.pledgeId, row2.pledgeRank);  // T-122: v12 columns

  Db db3;
  REQUIRE(db3.open(path, &err));
  CharacterRow row3;
  REQUIRE(db3.loginOrCreate("skilltester", "pw123", &row3, &reason, &err));
  CHECK(row3.swordSkill == 25);
  CHECK(row3.swingLands == 625);

  rmDb(path);
}

TEST_CASE("weapon skill persist: old rows default to 0 after migration") {
  const std::string path = tmpDbPath("bh_skill_default.bhdb");
  rmDb(path);

  // Create v10 DB with a character, then migrate
  sqlite3* raw = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &raw) == SQLITE_OK);
  char* msg = nullptr;
  REQUIRE(sqlite3_exec(raw,
    "CREATE TABLE IF NOT EXISTS accounts (id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE COLLATE NOCASE, salt INTEGER NOT NULL, pwhash INTEGER NOT NULL, created INTEGER NOT NULL DEFAULT (strftime('%s','now')));"
    "CREATE TABLE IF NOT EXISTS characters (id INTEGER PRIMARY KEY, account_id INTEGER NOT NULL REFERENCES accounts(id), name TEXT NOT NULL, map_id INTEGER NOT NULL DEFAULT 1, x INTEGER NOT NULL DEFAULT 0, y INTEGER NOT NULL DEFAULT 0, level INTEGER NOT NULL DEFAULT 1, xp INTEGER NOT NULL DEFAULT 0, created INTEGER NOT NULL DEFAULT (strftime('%s','now')), str INTEGER NOT NULL DEFAULT 8, vit INTEGER NOT NULL DEFAULT 8, dex INTEGER NOT NULL DEFAULT 8, stat_points INTEGER NOT NULL DEFAULT 0, gold INTEGER NOT NULL DEFAULT 50, inv TEXT NOT NULL DEFAULT '', anvil_mercy INTEGER NOT NULL DEFAULT 0, karma INTEGER NOT NULL DEFAULT 0, class_id INTEGER NOT NULL DEFAULT 1);"
    "INSERT INTO accounts(id, name, salt, pwhash) VALUES(1, 'oldguy', 123, 456);"
    "INSERT INTO characters(id, account_id, name, level, xp) VALUES(1, 1, 'oldguy', 5, 100);"
    "PRAGMA user_version=10;",
    nullptr, nullptr, &msg) == SQLITE_OK);
  sqlite3_close(raw);

  // Now open via Db — migration should set defaults 0, and we can insert new skill via saveProgress
  Db db;
  std::string err;
  REQUIRE(db.open(path, &err));

  // Directly query the migrated row's new columns
  sqlite3* check = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &check) == SQLITE_OK);
  sqlite3_stmt* st = nullptr;
  REQUIRE(sqlite3_prepare_v2(check, "SELECT sword_skill, swing_lands FROM characters WHERE id=1;", -1, &st, nullptr) == SQLITE_OK);
  REQUIRE(sqlite3_step(st) == SQLITE_ROW);
  CHECK(sqlite3_column_int(st, 0) == 0);
  CHECK(sqlite3_column_int64(st, 1) == 0);
  sqlite3_finalize(st);
  sqlite3_close(check);

  rmDb(path);
}
