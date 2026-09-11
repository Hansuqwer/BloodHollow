#include "persist.h"
#include <cstring>

#include <cstdio>
#include <filesystem>
#include <system_error>

#include "sim/bhmap.h"  // fnv1a64

namespace bh::server {

std::uint64_t stubPasswordHash(std::uint64_t salt, const std::string& pass) {
  // ADR-0009: deliberate stub. Iterated salted FNV-1a. Replace with argon2id
  // (and rate-limited login attempts) before opening to strangers.
  std::uint64_t h = salt;
  for (int i = 0; i < 4096; ++i) {
    h = sim::fnv1a64(reinterpret_cast<const std::uint8_t*>(pass.data()), pass.size(), h);
    h ^= h >> 13;
    h *= 0x100000001b3ULL;
  }
  return h;
}

bool Db::exec(const char* sql, std::string* err) {
  char* msg = nullptr;
  if (sqlite3_exec(db_, sql, nullptr, nullptr, &msg) != SQLITE_OK) {
    if (err) *err = msg != nullptr ? msg : "sqlite error";
    sqlite3_free(msg);
    return false;
  }
  return true;
}

bool Db::open(const std::string& path, std::string* err) {
  std::error_code ec;
  const auto parent = std::filesystem::path(path).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent, ec);
  if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
    if (err) *err = "sqlite3_open failed: " + path;
    return false;
  }
  if (!exec("PRAGMA journal_mode=WAL;", err) ||
      !exec("PRAGMA synchronous=NORMAL;", err)) {
    return false;
  }
  if (!exec(
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
      "  created INTEGER NOT NULL DEFAULT (strftime('%s','now'))"
      ");",
      err)) {
    return false;
  }
  // schema migrations (user_version guarded, additive only; table must exist)
  int uv = 0;
  {
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, "PRAGMA user_version;", -1, &st, nullptr) == SQLITE_OK &&
        sqlite3_step(st) == SQLITE_ROW) {
      uv = sqlite3_column_int(st, 0);
    }
    sqlite3_finalize(st);
  }
  if (uv < 2) {
    // v2 (Sprint 5): progression columns
    if (!exec("ALTER TABLE characters ADD COLUMN str INTEGER NOT NULL DEFAULT 8;"
              "ALTER TABLE characters ADD COLUMN vit INTEGER NOT NULL DEFAULT 8;"
              "ALTER TABLE characters ADD COLUMN dex INTEGER NOT NULL DEFAULT 8;"
              "ALTER TABLE characters ADD COLUMN stat_points INTEGER NOT NULL DEFAULT 0;",
              err)) {
      return false;
    }
    if (!exec("PRAGMA user_version=2;", err)) return false;
    uv = 2;
  }
  if (uv < 3) {
    // v3 (Sprint 6): gold + inventory blob
    if (!exec("ALTER TABLE characters ADD COLUMN gold INTEGER NOT NULL DEFAULT 50;"
              "ALTER TABLE characters ADD COLUMN inv TEXT NOT NULL DEFAULT '';",
              err)) {
      return false;
    }
    if (!exec("PRAGMA user_version=3;", err)) return false;
  }
  if (uv < 4) {
    // v4 (Sprint 8 / T-039): zone persistence; base CREATE already carries
    // map_id, so only ALTER when the column is genuinely absent (old files).
    bool hasMapId = false;
    {
      sqlite3_stmt* st = nullptr;
      if (sqlite3_prepare_v2(db_, "PRAGMA table_info(characters);", -1, &st,
                             nullptr) == SQLITE_OK) {
        while (sqlite3_step(st) == SQLITE_ROW) {
          const unsigned char* cn = sqlite3_column_text(st, 1);
          if (cn != nullptr && std::strcmp(reinterpret_cast<const char*>(cn),
                                           "map_id") == 0) {
            hasMapId = true;
            break;
          }
        }
      }
      sqlite3_finalize(st);
    }
    if (!hasMapId && !exec("ALTER TABLE characters ADD COLUMN map_id INTEGER NOT NULL DEFAULT 1;",
                           err)) {
      return false;
    }
    if (!exec("PRAGMA user_version=4;", err)) return false;
  }
  if (uv < 5) {
    // v5 (Sprint 9 / T-043): item-aura persistence lane (inv blob gains a 4th
    // `aura` field per record) + anvil honor bookkeeping.
    if (!exec("ALTER TABLE characters ADD COLUMN anvil_mercy INTEGER NOT NULL DEFAULT 0;",
              err)) {
      return false;
    }
    if (!exec("PRAGMA user_version=5;", err)) return false;
  }
  if (uv < 6) {
    // v6 (Sprint 9 / T-046): moral economy column.
    if (!exec("ALTER TABLE characters ADD COLUMN karma INTEGER NOT NULL DEFAULT 0;",
              err)) {
      return false;
    }
    if (!exec("PRAGMA user_version=6;", err)) return false;
  }
  if (uv < 7) {
    // v7 (Sprint 14 / T-053): class kit column; existing = Ravager (freeze).
    if (!exec("ALTER TABLE characters ADD COLUMN class_id INTEGER NOT NULL DEFAULT 1;",
              err)) {
      return false;
    }
    if (!exec("PRAGMA user_version=7;", err)) return false;
  }
  if (uv < 8) {
    // v8 (Sprint 16 / T-058): inv blob gains a 5th `durability` field per
    // record (column-free, parser-backward-compatible — no ALTER needed).
    if (!exec("PRAGMA user_version=8;", err)) return false;
  }
  if (uv < 9) {
    // v9 (Sprint 16 / T-059): inv blob gains a 6th `affix` field per record.
    if (!exec("PRAGMA user_version=9;", err)) return false;
  }
  if (uv < 10) {
    // v10 (Sprint 16 / T-060): inv blob gains a 7th `refine` field per record.
    if (!exec("PRAGMA user_version=10;", err)) return false;
  }
  return true;
}

Db::~Db() {
  if (db_ != nullptr) sqlite3_close(db_);
}

bool Db::accountExists(const std::string& user, bool* outExists,
                       std::string* err) {
  if (outExists == nullptr) return false;
  *outExists = false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(
          db_, "SELECT 1 FROM accounts WHERE name=? LIMIT 1;", -1, &st,
          nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (account exists)";
    return false;
  }
  sqlite3_bind_text(st, 1, user.c_str(), -1, SQLITE_TRANSIENT);
  const int rc = sqlite3_step(st);
  sqlite3_finalize(st);
  if (rc == SQLITE_ROW) {
    *outExists = true;
    return true;
  }
  if (rc != SQLITE_DONE) {
    if (err) *err = "step failed (account exists)";
    return false;
  }
  return true;
}

bool Db::loginOrCreate(const std::string& user, const std::string& pass,
                       CharacterRow* out, std::uint8_t* failReason, std::string* err) {
  if (db_ == nullptr) {
    if (err) *err = "db not open";
    return false;
  }
  if (user.size() < 3 || user.size() > 16) {
    *failReason = 2;
    return false;
  }
  for (const char c : user) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_' || c == '-';
    if (!ok) {
      *failReason = 2;
      return false;
    }
  }

  sqlite3_stmt* st = nullptr;
  bool ok = false;
  if (sqlite3_prepare_v2(db_, "SELECT id, salt, pwhash FROM accounts WHERE name=?;", -1, &st,
                         nullptr) != SQLITE_OK) {
    *failReason = 3;
    if (err) *err = "prepare failed (accounts)";
    return false;
  }
  sqlite3_bind_text(st, 1, user.c_str(), -1, SQLITE_TRANSIENT);
  const int rc = sqlite3_step(st);
  std::int64_t accountId = 0;
  if (rc == SQLITE_ROW) {
    accountId = sqlite3_column_int64(st, 0);
    const std::uint64_t salt = static_cast<std::uint64_t>(sqlite3_column_int64(st, 1));
    const std::uint64_t pw = static_cast<std::uint64_t>(sqlite3_column_int64(st, 2));
    sqlite3_finalize(st);
    if (stubPasswordHash(salt, pass) != pw) {
      *failReason = 1;
      return false;
    }
    ok = true;
  } else {
    sqlite3_finalize(st);
  }

  if (!ok) {
    // First sight of this user: create account.
    std::uint64_t salt = sim::fnv1a64(
        reinterpret_cast<const std::uint8_t*>(user.data()), user.size(), 0);
    salt ^= static_cast<std::uint64_t>(::time(nullptr)) * 2654435761ULL;
    sqlite3_stmt* ins = nullptr;
    if (sqlite3_prepare_v2(db_,
                           "INSERT INTO accounts(name, salt, pwhash) VALUES(?,?,?);", -1, &ins,
                           nullptr) != SQLITE_OK) {
      *failReason = 3;
      if (err) *err = "prepare failed (insert account)";
      return false;
    }
    sqlite3_bind_text(ins, 1, user.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(ins, 2, static_cast<sqlite3_int64>(salt));
    sqlite3_bind_int64(ins, 3,
                       static_cast<sqlite3_int64>(stubPasswordHash(salt, pass)));
    if (sqlite3_step(ins) != SQLITE_DONE) {
      sqlite3_finalize(ins);
      *failReason = 1;  // raced by another login; caller retries
      return false;
    }
    sqlite3_finalize(ins);
    accountId = sqlite3_last_insert_rowid(db_);
  }

  // v0: exactly one character per account, auto-created with the account name.
  sqlite3_stmt* cs = nullptr;
  if (sqlite3_prepare_v2(db_,
                         "SELECT id, name, map_id, x, y, level, xp, str, vit, dex, stat_points, gold, inv, anvil_mercy, karma, class_id "
        "FROM characters "
                         "WHERE account_id=? LIMIT 1;",
                         -1, &cs, nullptr) != SQLITE_OK) {
    *failReason = 3;
    if (err) *err = "prepare failed (characters)";
    return false;
  }
  sqlite3_bind_int64(cs, 1, accountId);
  if (sqlite3_step(cs) == SQLITE_ROW) {
    out->id = sqlite3_column_int64(cs, 0);
    out->name = reinterpret_cast<const char*>(sqlite3_column_text(cs, 1));
    out->mapId = sqlite3_column_int(cs, 2);
    out->x = sqlite3_column_int(cs, 3);
    out->y = sqlite3_column_int(cs, 4);
    out->level = sqlite3_column_int(cs, 5);
    out->xp = sqlite3_column_int64(cs, 6);
    out->str = sqlite3_column_int(cs, 7);
    out->vit = sqlite3_column_int(cs, 8);
    out->dex = sqlite3_column_int(cs, 9);
    out->statPoints = sqlite3_column_int(cs, 10);
    out->gold = sqlite3_column_int(cs, 11);
    const unsigned char* invTxt = sqlite3_column_text(cs, 12);
    out->invBlob = invTxt != nullptr ? reinterpret_cast<const char*>(invTxt) : "";
    out->anvilMercy = sqlite3_column_int64(cs, 13);
    out->karma = sqlite3_column_int(cs, 14);
    out->classId = sqlite3_column_int(cs, 15);
    sqlite3_finalize(cs);
    return true;
  }
  sqlite3_finalize(cs);

  sqlite3_stmt* ci = nullptr;
  if (sqlite3_prepare_v2(
          db_, "INSERT INTO characters(account_id, name, map_id, x, y) VALUES(?,?,1,0,0);", -1,
          &ci, nullptr) != SQLITE_OK) {
    *failReason = 3;
    if (err) *err = "prepare failed (insert character)";
    return false;
  }
  sqlite3_bind_int64(ci, 1, accountId);
  sqlite3_bind_text(ci, 2, user.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(ci) != SQLITE_DONE) {
    sqlite3_finalize(ci);
    *failReason = 3;
    if (err) *err = "insert character failed";
    return false;
  }
  sqlite3_finalize(ci);
  out->id = sqlite3_last_insert_rowid(db_);
  out->name = user;
  out->mapId = 1;
  out->x = 0;
  out->y = 0;  // 0,0 => server spawn point
  return true;
}

void Db::saveProgress(std::int64_t characterId, int level, std::int64_t xp, int str,
                      int vit, int dex, int statPoints, int gold,
                      const std::string& invBlob, std::int64_t anvilMercy,
                      std::int32_t karma, int classId) {
  if (db_ == nullptr) return;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_,
                         "UPDATE characters SET level=?, xp=?, str=?, vit=?, dex=?, "
                         "stat_points=?, gold=?, inv=?, anvil_mercy=?, karma=?, class_id=? WHERE id=?;",
                         -1, &st, nullptr) != SQLITE_OK) {
    return;
  }
  sqlite3_bind_int(st, 1, level);
  sqlite3_bind_int64(st, 2, xp);
  sqlite3_bind_int(st, 3, str);
  sqlite3_bind_int(st, 4, vit);
  sqlite3_bind_int(st, 5, dex);
  sqlite3_bind_int(st, 6, statPoints);
  sqlite3_bind_int(st, 7, gold);
  sqlite3_bind_text(st, 8, invBlob.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(st, 9, anvilMercy);
  sqlite3_bind_int(st, 10, karma);
  sqlite3_bind_int(st, 11, classId);
  sqlite3_bind_int64(st, 12, characterId);
  sqlite3_step(st);
  sqlite3_finalize(st);
}

void Db::savePosition(std::int64_t characterId, int mapId, int x, int y) {
  if (db_ == nullptr) return;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_, "UPDATE characters SET map_id=?, x=?, y=? WHERE id=?;", -1, &st,
                         nullptr) != SQLITE_OK) {
    return;
  }
  sqlite3_bind_int(st, 1, mapId);
  sqlite3_bind_int(st, 2, x);
  sqlite3_bind_int(st, 3, y);
  sqlite3_bind_int64(st, 4, characterId);
  sqlite3_step(st);
  sqlite3_finalize(st);
}

}  // namespace bh::server
