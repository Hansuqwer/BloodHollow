#pragma once

#include <cstdint>
#include <string>

#include <sqlite3.h>

namespace bh::server {

struct CharacterRow {
  std::int64_t id = 0;
  std::string name{};
  int mapId = 1;
  int x = 0;
  int y = 0;
  int level = 1;
  std::int64_t xp = 0;
  int str = 8;
  int vit = 8;
  int dex = 8;
  int statPoints = 0;
  int gold = 50;
  std::int64_t anvilMercy = 0;
  std::int32_t karma = 0;
  int classId = 1;  // schema v7 (S14): kKit* in content/kits.h
  int swordSkill = 0;           // schema v11 (T-118): use-based skill
  std::int64_t swingLands = 0;  // schema v11: exact land counter
  int townId = 0;  // schema v12 (T-130): 0 unsworn, 1 Thornwall, 2 Marrowgate
  int ek = 0;      // schema v12: enemy-kill fame (persisted, board-read)
  std::string invBlob{};  // "itemId:qty:equipped;..." (schema v3)
};

// SQLite (WAL) persistence, ADR-0004. Login flow for M1 is intentionally a
// stub (ADR-0009): per-account salted iterative FNV-1a — NOT cryptographically
// secure; replaced by argon2id before any public alpha wave.
class Db {
 public:
  bool open(const std::string& path, std::string* err);
  ~Db();

  // Login with auto-registration: unknown user => account + character created.
  // failReason: 1=bad credentials, 2=invalid name, 3=server/db error.
  bool loginOrCreate(const std::string& user, const std::string& pass,
                     CharacterRow* out, std::uint8_t* failReason, std::string* err);
  // T-109: registration-gate pre-check — is there already an account for this
  // name? The shell routes unknown names through --no-register and the
  // registration rate limiter BEFORE loginOrCreate can create anything.
  bool accountExists(const std::string& user, bool* outExists, std::string* err);
  void savePosition(std::int64_t characterId, int mapId, int x, int y);
  void saveProgress(std::int64_t characterId, int level, std::int64_t xp, int str,
                    int vit, int dex, int statPoints, int gold,
                    const std::string& invBlob, std::int64_t anvilMercy,
                    std::int32_t karma, int classId, int swordSkill,
                    std::int64_t swingLands, int townId, int ek);
  // T-134 siege_state (id=1 row): castle holder + tax vault + crown count.
  // Created IF NOT EXISTS on open (no user_version change — characters
  // ladder untouched). Empty table loads as all-zeros.
  struct SiegeRow {
    std::int64_t holderId = 0;
    std::string holderName{};
    std::int64_t vaultGold = 0;
    std::int64_t crowns = 0;
  };
  bool loadSiege(SiegeRow* out, std::string* err);
  bool saveSiege(std::int64_t holderId, const std::string& holderName,
                 std::int64_t vaultGold, std::int64_t crowns, std::string* err);

 private:
  sqlite3* db_ = nullptr;
  bool exec(const char* sql, std::string* err);
};

std::uint64_t stubPasswordHash(std::uint64_t salt, const std::string& pass);

}  // namespace bh::server
