#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

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
  int pledgeId = 0;             // schema v12 (T-122): pledge membership
  int pledgeRank = 0;           // 0 none / 1 Initiate / 2 Bloodsworn / 3 Liege
  std::string invBlob{};  // "itemId:qty:equipped;..." (schema v3)
};

// T-122 pledge-lite registry row (members are derived from characters.pledge_id)
struct PledgeRec {
  std::uint32_t id = 0;
  std::string name;
  int emblem = 0;
  std::string liege;
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
                    std::int64_t swingLands, int pledgeId, int pledgeRank);
  // T-122 pledge-lite: registry load/persist (live shell only; replay never
  // opens a Db). Membership columns ride saveProgress.
  bool loadPledges(std::vector<PledgeRec>* out, std::string* err);
  // memberships: (character name, {pledgeId, rank}) for pledge_id != 0
  bool loadPledgeMembers(
      std::vector<std::pair<std::string, std::pair<int, int>>>* out,
      std::string* err);
  bool upsertPledge(const PledgeRec& p, std::string* err);
  bool deletePledge(std::uint32_t id, std::string* err);

 private:
  sqlite3* db_ = nullptr;
  bool exec(const char* sql, std::string* err);
};

std::uint64_t stubPasswordHash(std::uint64_t salt, const std::string& pass);

}  // namespace bh::server
