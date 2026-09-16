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
  if (uv < 11) {
    // v11 (T-118): weapon-skill persistence — sword skill + exact swing lands.
    // Additive-only, defaults 0 for old rows.
    if (!exec("ALTER TABLE characters ADD COLUMN sword_skill INTEGER NOT NULL DEFAULT 0;"
              "ALTER TABLE characters ADD COLUMN swing_lands INTEGER NOT NULL DEFAULT 0;",
              err)) {
      return false;
    }
    if (!exec("PRAGMA user_version=11;", err)) return false;
    uv = 11;
  }
  if (uv < 12) {
    // v12 (T-130): town war identity + EK fame. Additive-only, defaults 0
    // (unsworn, no kills) for old rows.
    if (!exec("ALTER TABLE characters ADD COLUMN town_id INTEGER NOT NULL DEFAULT 0;"
              "ALTER TABLE characters ADD COLUMN ek INTEGER NOT NULL DEFAULT 0;",
              err)) {
      return false;
    }
    if (!exec("PRAGMA user_version=12;", err)) return false;
    uv = 12;
  }
  // T-134 siege_state: castle holder + tax vault (version-free table —
  // IF NOT EXISTS, characters ladder untouched).
  if (!exec("CREATE TABLE IF NOT EXISTS siege_state ("
            "  id INTEGER PRIMARY KEY CHECK (id = 1),"
            "  holder_id INTEGER NOT NULL DEFAULT 0,"
            "  holder_name TEXT NOT NULL DEFAULT '',"
            "  vault_gold INTEGER NOT NULL DEFAULT 0,"
            "  crowns INTEGER NOT NULL DEFAULT 0,"
            "  updated INTEGER NOT NULL DEFAULT (strftime('%s','now'))"
            ");",
            err)) {
    return false;
  }
  if (uv < 13) {
    // v13 (T-138 rebase of T-122): pledge-lite — membership columns + the
    // pledges registry. Additive-only; old rows default to unaffiliated.
    if (!exec("ALTER TABLE characters ADD COLUMN pledge_id INTEGER NOT NULL DEFAULT 0;"
              "ALTER TABLE characters ADD COLUMN pledge_rank INTEGER NOT NULL DEFAULT 0;"
              "CREATE TABLE IF NOT EXISTS pledges ("
              "id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE, "
              "emblem INTEGER NOT NULL DEFAULT 0, liege TEXT NOT NULL DEFAULT '');",
              err)) {
      return false;
    }
    if (!exec("PRAGMA user_version=13;", err)) return false;
    uv = 13;
  }
  if (uv < 14) {
    // v14 (T-140): pledge vault — tax-only pool (deposit-only MVP). The
    // pledges table always exists here (v13 above runs first on old DBs).
    if (!exec("ALTER TABLE pledges ADD COLUMN vault_gold INTEGER NOT NULL DEFAULT 0;",
              err)) {
      return false;
    }
    if (!exec("PRAGMA user_version=14;", err)) return false;
    uv = 14;
  }
  if (uv < 15) {
    // v15 (wave-2: T-167/T-161/T-166): creation identity + rebate stamps +
    // bounty mark. All additive with era defaults (sex 0 = legacy unknown,
    // ticks -1/0 = never, bounty 0 = none posted). T-153 (auth) takes v16.
    if (!exec("ALTER TABLE characters ADD COLUMN sex INTEGER NOT NULL DEFAULT 0;"
              "ALTER TABLE characters ADD COLUMN last_death_tick INTEGER NOT NULL DEFAULT -1;"
              "ALTER TABLE characters ADD COLUMN last_debt_xp INTEGER NOT NULL DEFAULT 0;"
              "ALTER TABLE characters ADD COLUMN last_res_tick INTEGER NOT NULL DEFAULT -7000;"
              "ALTER TABLE characters ADD COLUMN bounty_mob INTEGER NOT NULL DEFAULT 0;"
              "ALTER TABLE characters ADD COLUMN bounty_cycle INTEGER NOT NULL DEFAULT 0;",
              err)) {
      return false;
    }
    if (!exec("PRAGMA user_version=15;", err)) return false;
    uv = 15;
  }
  // T-152: bans + gm_accounts (version-free, IF NOT EXISTS — like siege_state —
  // so old journals keep epoch 28; no user_version bump required).
  if (!exec("CREATE TABLE IF NOT EXISTS bans ("
            "  name TEXT PRIMARY KEY COLLATE NOCASE,"
            "  expires INTEGER NOT NULL,"
            "  reason TEXT NOT NULL DEFAULT '',"
            "  banned_by TEXT NOT NULL DEFAULT '',"
            "  created INTEGER NOT NULL DEFAULT (strftime('%s','now'))"
            ");"
            "CREATE TABLE IF NOT EXISTS gm_accounts ("
            "  name TEXT PRIMARY KEY COLLATE NOCASE,"
            "  added_by TEXT NOT NULL DEFAULT '',"
            "  created INTEGER NOT NULL DEFAULT (strftime('%s','now'))"
            ");",
            err)) {
    return false;
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
                       CharacterRow* out, std::uint8_t* failReason, std::string* err,
                       bool* freshOut) {
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

  // v11: include sword_skill + swing_lands (additive, defaults 0 for old DBs
  // after migration). The SELECT is prepared AFTER open() migrations, so
  // columns are guaranteed to exist on a migrated file.
  // v12 (T-130): town_id + ek ride the same guarantee.
  // v15 (wave-2): sex + rebate stamps + bounty mark ride it too.
  sqlite3_stmt* cs = nullptr;
  if (sqlite3_prepare_v2(db_,
                         "SELECT id, name, map_id, x, y, level, xp, str, vit, dex, stat_points, gold, inv, anvil_mercy, karma, class_id, sword_skill, swing_lands, town_id, ek, pledge_id, pledge_rank, sex, last_death_tick, last_debt_xp, last_res_tick, bounty_mob, bounty_cycle "

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
    out->swordSkill = sqlite3_column_int(cs, 16);
    out->swingLands = sqlite3_column_int64(cs, 17);
    out->townId = sqlite3_column_int(cs, 18);
    out->ek = sqlite3_column_int(cs, 19);
    out->pledgeId = sqlite3_column_int(cs, 20);  // v13 (T-138 rebase)
    out->pledgeRank = sqlite3_column_int(cs, 21);
    out->sex = sqlite3_column_int(cs, 22);  // v15 (T-167)
    out->lastDeathTick = sqlite3_column_int64(cs, 23);  // v15 (T-161)
    out->lastDebtXp = sqlite3_column_int64(cs, 24);
    out->lastResTick = sqlite3_column_int64(cs, 25);
    out->bountyMob = sqlite3_column_int(cs, 26);  // v15 (T-166)
    out->bountyCycle = sqlite3_column_int(cs, 27);
    sqlite3_finalize(cs);
    if (freshOut != nullptr) *freshOut = false;
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
  // defaults for new row: skill 0, lands 0 (already in CharacterRow)
  out->swordSkill = 0;
  out->swingLands = 0;
  out->townId = 0;  // unsworn (T-130)
  out->ek = 0;
  out->classId = 0;  // T-167: Unsworn until the creation panel answers
  out->sex = 0;      // T-167: unknown until chosen (v15 defaults match)
  out->lastDeathTick = -1;
  out->lastDebtXp = 0;
  out->lastResTick = -7000;
  out->bountyMob = 0;
  out->bountyCycle = 0;
  if (freshOut != nullptr) *freshOut = true;
  return true;
}

bool Db::setCreation(std::int64_t characterId, int classId, int sex,
                     std::string* err) {
  if (db_ == nullptr) {
    if (err) *err = "db not open";
    return false;
  }
  if (classId < 1 || classId > 3 || sex < 1 || sex > 2) {
    if (err) *err = "creation out of domain";
    return false;
  }
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_, "UPDATE characters SET class_id=?, sex=? WHERE id=?;",
                         -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (set creation)";
    return false;
  }
  sqlite3_bind_int(st, 1, classId);
  sqlite3_bind_int(st, 2, sex);
  sqlite3_bind_int64(st, 3, characterId);
  const bool ok = sqlite3_step(st) == SQLITE_DONE && sqlite3_changes(db_) == 1;
  sqlite3_finalize(st);
  if (!ok && err) *err = "creation update missed";
  return ok;
}

bool Db::loginByRowId(std::int64_t characterId, CharacterRow* out,
                      std::uint8_t* failReason, std::string* err) {
  if (db_ == nullptr) {
    if (err) *err = "db not open";
    return false;
  }
  sqlite3_stmt* cs = nullptr;
  if (sqlite3_prepare_v2(db_,
                         "SELECT id, name, map_id, x, y, level, xp, str, vit, dex, stat_points, gold, inv, anvil_mercy, karma, class_id, sword_skill, swing_lands, town_id, ek, pledge_id, pledge_rank, sex, last_death_tick, last_debt_xp, last_res_tick, bounty_mob, bounty_cycle "
                         "FROM characters WHERE id=? LIMIT 1;",
                         -1, &cs, nullptr) != SQLITE_OK) {
    *failReason = 3;
    if (err) *err = "prepare failed (characters by id)";
    return false;
  }
  sqlite3_bind_int64(cs, 1, characterId);
  if (sqlite3_step(cs) != SQLITE_ROW) {
    sqlite3_finalize(cs);
    *failReason = 3;
    if (err) *err = "character row gone";
    return false;
  }
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
  out->swordSkill = sqlite3_column_int(cs, 16);
  out->swingLands = sqlite3_column_int64(cs, 17);
  out->townId = sqlite3_column_int(cs, 18);
  out->ek = sqlite3_column_int(cs, 19);
  out->pledgeId = sqlite3_column_int(cs, 20);
  out->pledgeRank = sqlite3_column_int(cs, 21);
  out->sex = sqlite3_column_int(cs, 22);
  out->lastDeathTick = sqlite3_column_int64(cs, 23);
  out->lastDebtXp = sqlite3_column_int64(cs, 24);
  out->lastResTick = sqlite3_column_int64(cs, 25);
  out->bountyMob = sqlite3_column_int(cs, 26);
  out->bountyCycle = sqlite3_column_int(cs, 27);
  sqlite3_finalize(cs);
  return true;
}

void Db::saveProgress(std::int64_t characterId, int level, std::int64_t xp, int str,
                      int vit, int dex, int statPoints, int gold,
                      const std::string& invBlob, std::int64_t anvilMercy,
                      std::int32_t karma, int classId, int swordSkill,
                      std::int64_t swingLands, int townId, int ek,
                      int pledgeId, int pledgeRank,
                      int sex, std::int64_t lastDeathTick, std::int64_t lastDebtXp,
                      std::int64_t lastResTick, int bountyMob, int bountyCycle) {
  if (db_ == nullptr) return;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_,
                         "UPDATE characters SET level=?, xp=?, str=?, vit=?, dex=?, "
                         "stat_points=?, gold=?, inv=?, anvil_mercy=?, karma=?, class_id=?, sword_skill=?, swing_lands=?, town_id=?, ek=?, pledge_id=?, pledge_rank=?, "
                         "sex=?, last_death_tick=?, last_debt_xp=?, last_res_tick=?, bounty_mob=?, bounty_cycle=? WHERE id=?;",
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
  sqlite3_bind_int(st, 12, swordSkill);
  sqlite3_bind_int64(st, 13, swingLands);
  sqlite3_bind_int(st, 14, townId);
  sqlite3_bind_int(st, 15, ek);
  sqlite3_bind_int(st, 16, pledgeId);  // v13 (T-138 rebase)
  sqlite3_bind_int(st, 17, pledgeRank);
  sqlite3_bind_int(st, 18, sex);  // v15 (wave-2: T-167/T-161/T-166)
  sqlite3_bind_int64(st, 19, lastDeathTick);
  sqlite3_bind_int64(st, 20, lastDebtXp);
  sqlite3_bind_int64(st, 21, lastResTick);
  sqlite3_bind_int(st, 22, bountyMob);
  sqlite3_bind_int(st, 23, bountyCycle);
  sqlite3_bind_int64(st, 24, characterId);
  sqlite3_step(st);
  sqlite3_finalize(st);
}

// T-134 siege_state: single-row castle memory (empty table => zeros).
bool Db::loadSiege(SiegeRow* out, std::string* err) {
  if (db_ == nullptr || out == nullptr) return false;
  *out = SiegeRow{};
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_,
                         "SELECT holder_id, holder_name, vault_gold, crowns "
                         "FROM siege_state WHERE id=1;",
                         -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (siege_state)";
    return false;
  }
  if (sqlite3_step(st) == SQLITE_ROW) {
    out->holderId = sqlite3_column_int64(st, 0);
    const unsigned char* nm = sqlite3_column_text(st, 1);
    out->holderName = nm != nullptr ? reinterpret_cast<const char*>(nm) : "";
    out->vaultGold = sqlite3_column_int64(st, 2);
    out->crowns = sqlite3_column_int64(st, 3);
  }
  sqlite3_finalize(st);
  return true;
}

// ---- T-122 pledge-lite registry I/O (live shell only) ----------------------
// T-140: SELECT carries vault_gold (v14; Db::open always migrates first).
bool Db::loadPledges(std::vector<PledgeRec>* out, std::string* err) {
  if (db_ == nullptr) return false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_, "SELECT id, name, emblem, liege, vault_gold FROM pledges;", -1,
                         &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (pledges)";
    return false;
  }
  while (sqlite3_step(st) == SQLITE_ROW) {
    PledgeRec p;
    p.id = static_cast<std::uint32_t>(sqlite3_column_int64(st, 0));
    p.name = reinterpret_cast<const char*>(sqlite3_column_text(st, 1));
    p.emblem = sqlite3_column_int(st, 2);
    p.liege = reinterpret_cast<const char*>(sqlite3_column_text(st, 3));
    p.vault = static_cast<std::uint32_t>(sqlite3_column_int64(st, 4));
    out->push_back(std::move(p));
  }
  sqlite3_finalize(st);
  return true;
}

bool Db::saveSiege(std::int64_t holderId, const std::string& holderName,
                   std::int64_t vaultGold, std::int64_t crowns,
                   std::string* err) {
  if (db_ == nullptr) return false;
  sqlite3_stmt* st = nullptr;
  // INSERT OR REPLACE (not ON CONFLICT — older sqlite compat): single row.
  if (sqlite3_prepare_v2(db_,
                         "INSERT OR REPLACE INTO siege_state(id, holder_id, holder_name, vault_gold, crowns) "
                         "VALUES(1,?,?,?,?);",
                         -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (siege_state)";
    return false;
  }
  sqlite3_bind_int64(st, 1, holderId);
  sqlite3_bind_text(st, 2, holderName.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(st, 3, vaultGold);
  sqlite3_bind_int64(st, 4, crowns);
  const bool ok = sqlite3_step(st) == SQLITE_DONE;
  if (!ok && err) *err = "write failed (siege_state)";
  sqlite3_finalize(st);
  return ok;
}

// ---- T-122/T-138 pledge-lite registry I/O (live shell only) ------

bool Db::loadPledgeMembers(
    std::vector<std::pair<std::string, std::pair<int, int>>>* out,
    std::string* err) {
  if (db_ == nullptr) return false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_,
                         "SELECT name, pledge_id, pledge_rank FROM characters "
                         "WHERE pledge_id != 0 ORDER BY name;",
                         -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (pledge members)";
    return false;
  }
  while (sqlite3_step(st) == SQLITE_ROW) {
    out->push_back({reinterpret_cast<const char*>(sqlite3_column_text(st, 0)),
                    {sqlite3_column_int(st, 1), sqlite3_column_int(st, 2)}});
  }
  sqlite3_finalize(st);
  return true;
}

bool Db::upsertPledge(const PledgeRec& p, std::string* err) {
  if (db_ == nullptr) return false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_,
                         "INSERT INTO pledges (id, name, emblem, liege, vault_gold) VALUES (?,?,?,?,?) "
                         "ON CONFLICT(id) DO UPDATE SET name=excluded.name, "
                         "emblem=excluded.emblem, liege=excluded.liege, "
                         "vault_gold=excluded.vault_gold;",
                         -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (pledges upsert)";
    return false;
  }
  sqlite3_bind_int64(st, 1, p.id);
  sqlite3_bind_text(st, 2, p.name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(st, 3, p.emblem);
  sqlite3_bind_text(st, 4, p.liege.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(st, 5, p.vault);
  const bool ok = sqlite3_step(st) == SQLITE_DONE;
  sqlite3_finalize(st);
  if (!ok && err) *err = "pledges upsert failed";
  return ok;
}

bool Db::deletePledge(std::uint32_t id, std::string* err) {
  if (db_ == nullptr) return false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_, "DELETE FROM pledges WHERE id=?;", -1, &st,
                         nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (pledges delete)";
    return false;
  }
  sqlite3_bind_int64(st, 1, id);
  const bool ok = sqlite3_step(st) == SQLITE_DONE;
  sqlite3_finalize(st);
  if (!ok && err) *err = "pledges delete failed";
  return ok;
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

// ---- T-152 bans + gm_accounts (version-free, like siege_state) -------------

bool Db::loadBans(std::vector<BanRec>* out, std::string* err) {
  if (db_ == nullptr) return false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_, "SELECT name, expires, reason, banned_by, created FROM bans ORDER BY name;", -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (bans load)";
    return false;
  }
  while (sqlite3_step(st) == SQLITE_ROW) {
    BanRec b;
    b.name = reinterpret_cast<const char*>(sqlite3_column_text(st, 0));
    b.expires = sqlite3_column_int64(st, 1);
    const unsigned char* rs = sqlite3_column_text(st, 2);
    b.reason = rs != nullptr ? reinterpret_cast<const char*>(rs) : "";
    const unsigned char* by = sqlite3_column_text(st, 3);
    b.bannedBy = by != nullptr ? reinterpret_cast<const char*>(by) : "";
    b.created = sqlite3_column_int64(st, 4);
    out->push_back(std::move(b));
  }
  sqlite3_finalize(st);
  return true;
}

bool Db::isBanned(const std::string& name, bool* out, std::string* reason,
                  std::int64_t* expires, std::string* err) {
  if (db_ == nullptr || out == nullptr) return false;
  *out = false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_, "SELECT expires, reason FROM bans WHERE name=? COLLATE NOCASE LIMIT 1;", -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (isBanned)";
    return false;
  }
  sqlite3_bind_text(st, 1, name.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(st) == SQLITE_ROW) {
    const std::int64_t exp = sqlite3_column_int64(st, 0);
    const unsigned char* rs = sqlite3_column_text(st, 1);
    const std::int64_t now = ::time(nullptr);
    if (exp == 0 || exp > now) {
      *out = true;
      if (reason != nullptr) reason->assign(rs != nullptr ? reinterpret_cast<const char*>(rs) : "");
      if (expires != nullptr) *expires = exp;
    } else {
      // expired: treat as not banned (caller may prune)
      *out = false;
    }
  }
  sqlite3_finalize(st);
  return true;
}

bool Db::upsertBan(const std::string& name, std::int64_t expires,
                   const std::string& reason, const std::string& by,
                   std::string* err) {
  if (db_ == nullptr) return false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_, "INSERT INTO bans(name, expires, reason, banned_by) VALUES(?,?,?,?) "
                              "ON CONFLICT(name) DO UPDATE SET expires=excluded.expires, reason=excluded.reason, banned_by=excluded.banned_by, created=strftime('%s','now');",
                         -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (upsertBan)";
    return false;
  }
  sqlite3_bind_text(st, 1, name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(st, 2, expires);
  sqlite3_bind_text(st, 3, reason.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(st, 4, by.c_str(), -1, SQLITE_TRANSIENT);
  const bool ok = sqlite3_step(st) == SQLITE_DONE;
  if (!ok && err) *err = "write failed (upsertBan)";
  sqlite3_finalize(st);
  return ok;
}

bool Db::deleteBan(const std::string& name, std::string* err) {
  if (db_ == nullptr) return false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_, "DELETE FROM bans WHERE name=? COLLATE NOCASE;", -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (deleteBan)";
    return false;
  }
  sqlite3_bind_text(st, 1, name.c_str(), -1, SQLITE_TRANSIENT);
  const bool ok = sqlite3_step(st) == SQLITE_DONE;
  if (!ok && err) *err = "delete failed (deleteBan)";
  sqlite3_finalize(st);
  return ok;
}

bool Db::pruneExpiredBans(std::string* err) {
  if (db_ == nullptr) return false;
  char* msg = nullptr;
  const std::int64_t now = ::time(nullptr);
  std::string sql = "DELETE FROM bans WHERE expires != 0 AND expires <= " + std::to_string(now) + ";";
  if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &msg) != SQLITE_OK) {
    if (err) *err = msg != nullptr ? msg : "prune failed";
    sqlite3_free(msg);
    return false;
  }
  return true;
}

bool Db::loadGmAccounts(std::vector<std::string>* out, std::string* err) {
  if (db_ == nullptr) return false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_, "SELECT name FROM gm_accounts ORDER BY name;", -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (gm_accounts)";
    return false;
  }
  while (sqlite3_step(st) == SQLITE_ROW) {
    const unsigned char* txt = sqlite3_column_text(st, 0);
    if (txt != nullptr) out->push_back(reinterpret_cast<const char*>(txt));
  }
  sqlite3_finalize(st);
  return true;
}

bool Db::upsertGmAccount(const std::string& name, std::string* err) {
  if (db_ == nullptr) return false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_, "INSERT INTO gm_accounts(name) VALUES(?) ON CONFLICT(name) DO NOTHING;", -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (gm upsert)";
    return false;
  }
  sqlite3_bind_text(st, 1, name.c_str(), -1, SQLITE_TRANSIENT);
  const bool ok = sqlite3_step(st) == SQLITE_DONE;
  if (!ok && err) *err = "write failed (gm upsert)";
  sqlite3_finalize(st);
  return ok;
}

bool Db::deleteGmAccount(const std::string& name, std::string* err) {
  if (db_ == nullptr) return false;
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(db_, "DELETE FROM gm_accounts WHERE name=? COLLATE NOCASE;", -1, &st, nullptr) != SQLITE_OK) {
    if (err) *err = "prepare failed (gm delete)";
    return false;
  }
  sqlite3_bind_text(st, 1, name.c_str(), -1, SQLITE_TRANSIENT);
  const bool ok = sqlite3_step(st) == SQLITE_DONE;
  if (!ok && err) *err = "delete failed (gm delete)";
  sqlite3_finalize(st);
  return ok;
}

}  // namespace bh::server
