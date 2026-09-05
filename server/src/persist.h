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
  void savePosition(std::int64_t characterId, int mapId, int x, int y);
    void saveProgress(std::int64_t characterId, int level, std::int64_t xp, int str,
                    int vit, int dex, int statPoints, int gold,
                    const std::string& invBlob, std::int64_t anvilMercy,
                    std::int32_t karma, int classId);

 private:
  sqlite3* db_ = nullptr;
  bool exec(const char* sql, std::string* err);
};

std::uint64_t stubPasswordHash(std::uint64_t salt, const std::string& pass);

}  // namespace bh::server
