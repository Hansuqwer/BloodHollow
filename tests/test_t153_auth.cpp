// T-153 argon2id (ADR-0012): PHC storage, stub migration, refusal paths.
#include <chrono>
#include <cstdint>
#include <filesystem>
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

std::string readPhc(const std::string& path, const std::string& user) {
  sqlite3* check = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &check) == SQLITE_OK);
  sqlite3_stmt* st = nullptr;
  REQUIRE(sqlite3_prepare_v2(check, "SELECT pwhash_phc FROM accounts WHERE name=?;",
                             -1, &st, nullptr) == SQLITE_OK);
  sqlite3_bind_text(st, 1, user.c_str(), -1, SQLITE_TRANSIENT);
  std::string out;
  if (sqlite3_step(st) == SQLITE_ROW) {
    const unsigned char* t = sqlite3_column_text(st, 0);
    out = t != nullptr ? reinterpret_cast<const char*>(t) : "";
  }
  sqlite3_finalize(st);
  sqlite3_close(check);
  return out;
}
}  // namespace

TEST_CASE("T-153: schema v16 migrates with the PHC column") {
  const std::string path = tmpDbPath("bh_auth_v16.bhdb");
  rmDb(path);
  Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  sqlite3* check = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &check) == SQLITE_OK);
  sqlite3_stmt* st = nullptr;
  REQUIRE(sqlite3_prepare_v2(check, "PRAGMA user_version;", -1, &st, nullptr) ==
          SQLITE_OK);
  REQUIRE(sqlite3_step(st) == SQLITE_ROW);
  CHECK(sqlite3_column_int(st, 0) == 16);
  sqlite3_finalize(st);
  sqlite3_close(check);
}

TEST_CASE("T-153: new accounts store argon2id PHC; wrong password refused") {
  const std::string path = tmpDbPath("bh_auth_new.bhdb");
  rmDb(path);
  Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  CharacterRow row;
  std::uint8_t reason = 0;
  REQUIRE(db.loginOrCreate("argonborn", "s3cret-ish", &row, &reason, &err));
  const std::string phc = readPhc(path, "argonborn");
  CHECK(phc.rfind("$argon2id$", 0) == 0);  // PHC self-describes id+params
  // wrong password: refused (reason 1, lockout accounting preserved)
  CharacterRow row2;
  CHECK_FALSE(db.loginOrCreate("argonborn", "wrongpw", &row2, &reason, &err));
  CHECK(reason == 1);
}

TEST_CASE("T-153: stub-era rows migrate silently on next login") {
  const std::string path = tmpDbPath("bh_auth_mig.bhdb");
  rmDb(path);
  // Simulate a stub survivor: create (argon), then WIPE the PHC column.
  // Correct password must still log in (stub verify) AND rehash.
  Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  CharacterRow row;
  std::uint8_t reason = 0;
  REQUIRE(db.loginOrCreate("survivor", "oldpass99", &row, &reason, &err));
  REQUIRE(readPhc(path, "survivor").rfind("$argon2id$", 0) == 0);
  // wipe PHC -> stub survivor; correct password still logs in AND rehashes
  {
    sqlite3* raw = nullptr;
    REQUIRE(sqlite3_open(path.c_str(), &raw) == SQLITE_OK);
    char* msg = nullptr;
    REQUIRE(sqlite3_exec(raw,
                         "UPDATE accounts SET pwhash_phc='' WHERE name='survivor';",
                         nullptr, nullptr, &msg) == SQLITE_OK);
    sqlite3_close(raw);
  }
  CharacterRow row2;
  REQUIRE(db.loginOrCreate("survivor", "oldpass99", &row2, &reason, &err));
  CHECK(readPhc(path, "survivor").rfind("$argon2id$", 0) == 0);
  // second login rides the argon path: corrupt the stub and it still passes
  {
    sqlite3* raw = nullptr;
    REQUIRE(sqlite3_open(path.c_str(), &raw) == SQLITE_OK);
    char* msg = nullptr;
    REQUIRE(sqlite3_exec(raw,
                         "UPDATE accounts SET pwhash=1 WHERE name='survivor';",
                         nullptr, nullptr, &msg) == SQLITE_OK);
    sqlite3_close(raw);
  }
  CharacterRow row3;
  REQUIRE(db.loginOrCreate("survivor", "oldpass99", &row3, &reason, &err));
}

TEST_CASE("T-153: malformed stored hash refuses without crashing") {
  const std::string path = tmpDbPath("bh_auth_bad.bhdb");
  rmDb(path);
  Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  CharacterRow row;
  std::uint8_t reason = 0;
  REQUIRE(db.loginOrCreate("garbage", "pw123456", &row, &reason, &err));
  {
    sqlite3* raw = nullptr;
    REQUIRE(sqlite3_open(path.c_str(), &raw) == SQLITE_OK);
    char* msg = nullptr;
    REQUIRE(sqlite3_exec(raw,
                         "UPDATE accounts SET pwhash_phc='not-a-hash!!!' WHERE name='garbage';",
                         nullptr, nullptr, &msg) == SQLITE_OK);
    sqlite3_close(raw);
  }
  CharacterRow row2;
  CHECK_FALSE(db.loginOrCreate("garbage", "pw123456", &row2, &reason, &err));
  CHECK(reason == 1);
}

TEST_CASE("T-153: verify cost is login-shaped (loose bound, prints ms)") {
  const std::string path = tmpDbPath("bh_auth_cost.bhdb");
  rmDb(path);
  Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  CharacterRow row;
  std::uint8_t reason = 0;
  auto t0 = std::chrono::steady_clock::now();
  REQUIRE(db.loginOrCreate("metered", "measure-me-00", &row, &reason, &err));
  auto t1 = std::chrono::steady_clock::now();
  REQUIRE(db.loginOrCreate("metered", "measure-me-00", &row, &reason, &err));
  auto t2 = std::chrono::steady_clock::now();
  const long long hashMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  const long long verifyMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
  MESSAGE("argon2id m=19456,t=2,p=1 hash=" << hashMs << "ms verify=" << verifyMs << "ms");
  CHECK(hashMs < 2000);    // sandbox-class box; production records its own
  CHECK(verifyMs < 2000);  // number in the PR (limiter spreads bursts anyway)
}
