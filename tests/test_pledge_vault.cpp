// T-140 pledge vault: deposit-only tax pool (tithe + sworn-holder drip).
// Pins: tithe matrix, readout line, drip routing (twin worlds), v13 -> v14
// migration + Db round-trip, journal parity through applyWorldCommand.
#include <doctest/doctest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "command.h"
#include "content/mobs.h"
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

void registrarNear(server::World& w, int x = 13, int y = 10) {
  (void)w.debugSpawnRegistrar(sim::TilePos{x, y});
}

// founder with a fresh pledge; returns the liege (gold 10000 post-toll).
server::Entity* found(server::World& w, const char* name, const char* pledge) {
  server::Entity* p = spawnP(w, name, 10, 10);
  registrarNear(w);
  p->level = 10;
  p->gold = 20000;
  REQUIRE(w.pledgeCreate(*p, pledge));
  return w.find(p->id);
}

std::string tmpDbPath(const char* tag) {
  return (std::filesystem::temp_directory_path() /
          (std::string("bh_vault_") + tag + ".bhdb"))
      .string();
}

void rmDb(const std::string& p) {
  std::filesystem::remove(p);
  std::filesystem::remove(p + "-wal");
  std::filesystem::remove(p + "-shm");
}
}  // namespace

TEST_CASE("T-140: tithe moves gold to the vault, refusal matrix quiet") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* liege = found(w, "liege", "Ashfall");
  REQUIRE(liege->gold == 10000);
  w.pledgesDirty = false;

  CHECK(w.pledgeTithe(*liege, 2500));
  CHECK(liege->gold == 7500);
  REQUIRE(w.pledgeById(liege->pledgeId) != nullptr);
  CHECK(w.pledgeById(liege->pledgeId)->vault == 2500);
  CHECK(w.pledgesDirty);  // shell mirrors the vault

  CHECK_FALSE(w.pledgeTithe(*liege, 0));        // nothing moves for free
  CHECK_FALSE(w.pledgeTithe(*liege, 7501));     // cannot tithe beyond purse
  CHECK(liege->gold == 7500);
  CHECK(w.pledgeById(liege->pledgeId)->vault == 2500);

  server::Entity* stray = spawnP(w, "stray", 11, 10);
  stray->gold = 5000;
  CHECK_FALSE(w.pledgeTithe(*stray, 100));  // the unsworn fund nothing
  stray->pledgeId = 999;                    // desynced cache, no registry
  CHECK_FALSE(w.pledgeTithe(*stray, 100));
  liege->dead = true;
  CHECK_FALSE(w.pledgeTithe(*liege, 100));  // the dead give nothing
}

TEST_CASE("T-140: vault readout names the pool (directed, unjournaled)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* liege = found(w, "liege", "Ashfall");
  REQUIRE(w.pledgeTithe(*liege, 3000));
  const std::size_t before = w.events().size();
  w.pledgeVaultReadout(*liege);
  REQUIRE(w.events().size() == before + 1);
  const auto& ev = w.events().back();
  CHECK(ev.chatCh == 255);
  CHECK(ev.aboutId == liege->id);
  CHECK(ev.chatText.find("Ashfall vault: 3000g") != std::string::npos);
  server::Entity* stray = spawnP(w, "stray", 11, 10);
  w.pledgeVaultReadout(*stray);
  CHECK(w.events().back().chatText.find("Unsworn") != std::string::npos);
}

TEST_CASE("T-140: sworn holder's drip feeds the pledge vault, not the castle") {
  // Twin worlds, hound prey; the ONLY difference is whether the seated
  // holder's name rides the pledge registry. Same stream => exact routing.
  auto run = [](bool sworn) {
    server::World w;
    w.loadFrom(makeArena());
    w.loadSiegeState(7, "Ashen", 0, 1);
    if (sworn) {
      server::World::Pledge p;
      p.id = 7;
      p.name = "Ashfall";
      p.liege = "Ashen";
      p.members.push_back("Ashen");
      std::vector<server::World::Pledge> loaded;
      loaded.push_back(std::move(p));
      w.setPledges(std::move(loaded));
    }
    server::Entity* p = spawnP(w, sworn ? "taxed" : "free", 5, 5);
    p->hpMax = 2000;
    p->hp = 2000;
    w.debugGive(*p, 2002, 1);  // Pit Blade
    for (std::uint8_t i = 0; i < p->inv.size(); ++i)
      if (p->inv[i].itemId == 2002) {
        w.toggleEquip(*p, i);
        break;
      }
    server::Entity* hound =
        w.find(w.debugSpawnMob(*content::findMob(1003), {6, 5}).id);
    const std::uint32_t hid = hound->id;
    const std::uint32_t pid = p->id;
    server::Command c;
    c.kind = server::Command::kAttack;
    c.a = static_cast<std::int32_t>(hid);
    server::applyWorldCommand(w, *w.find(pid), c);
    int guard = 600;
    while (w.find(hid) != nullptr && guard-- > 0) w.tick();
    REQUIRE(w.find(hid) == nullptr);
    const server::World::Pledge* pledge = sworn ? w.pledgeById(7) : nullptr;
    return std::pair<std::uint32_t, std::uint32_t>(
        w.siegeVault(), pledge != nullptr ? pledge->vault : 0u);
  };
  const auto [castleFree, pledgeFree] = run(false);
  const auto [castleSworn, pledgeSworn] = run(true);
  REQUIRE(castleFree >= 1u);   // unsworn holder: castle pool eats (T-134 law)
  CHECK(pledgeFree == 0u);
  CHECK(castleSworn == 0u);    // sworn holder: the oath eats instead
  CHECK(pledgeSworn == castleFree);  // same stream, same tithe, new pocket
}

TEST_CASE("T-140: v13 -> v14 migration adds vault_gold defaulting 0") {
  const std::string path = tmpDbPath("mig");
  rmDb(path);
  sqlite3* raw = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &raw) == SQLITE_OK);
  REQUIRE(sqlite3_exec(raw,
                       "CREATE TABLE IF NOT EXISTS accounts (id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE COLLATE NOCASE, salt INTEGER NOT NULL, pwhash INTEGER NOT NULL, created INTEGER NOT NULL DEFAULT (strftime('%s','now')));"
                       "CREATE TABLE IF NOT EXISTS characters (id INTEGER PRIMARY KEY, account_id INTEGER NOT NULL REFERENCES accounts(id), name TEXT NOT NULL, map_id INTEGER NOT NULL DEFAULT 1, x INTEGER NOT NULL DEFAULT 0, y INTEGER NOT NULL DEFAULT 0, level INTEGER NOT NULL DEFAULT 1, xp INTEGER NOT NULL DEFAULT 0, created INTEGER NOT NULL DEFAULT (strftime('%s','now')), str INTEGER NOT NULL DEFAULT 8, vit INTEGER NOT NULL DEFAULT 8, dex INTEGER NOT NULL DEFAULT 8, stat_points INTEGER NOT NULL DEFAULT 0, gold INTEGER NOT NULL DEFAULT 50, inv TEXT NOT NULL DEFAULT '', anvil_mercy INTEGER NOT NULL DEFAULT 0, karma INTEGER NOT NULL DEFAULT 0, class_id INTEGER NOT NULL DEFAULT 1, sword_skill INTEGER NOT NULL DEFAULT 0, swing_lands INTEGER NOT NULL DEFAULT 0, town_id INTEGER NOT NULL DEFAULT 0, ek INTEGER NOT NULL DEFAULT 0, pledge_id INTEGER NOT NULL DEFAULT 0, pledge_rank INTEGER NOT NULL DEFAULT 0);"
                       "CREATE TABLE IF NOT EXISTS pledges (id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE, emblem INTEGER NOT NULL DEFAULT 0, liege TEXT NOT NULL DEFAULT '');"
                       "INSERT INTO pledges(id, name, emblem, liege) VALUES(7, 'Ashfall', 3, 'liege');"
                       "PRAGMA user_version=13;",
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
  CHECK(sqlite3_column_int(st, 0) == 14);
  sqlite3_finalize(st);
  REQUIRE(sqlite3_prepare_v2(check,
                             "SELECT vault_gold FROM pledges WHERE id=7;",
                             -1, &st, nullptr) == SQLITE_OK);
  REQUIRE(sqlite3_step(st) == SQLITE_ROW);
  CHECK(sqlite3_column_int64(st, 0) == 0);
  sqlite3_finalize(st);
  sqlite3_close(check);

  // vault round-trips through upsert/load
  std::vector<server::PledgeRec> loaded;
  REQUIRE(db.loadPledges(&loaded, &err));
  REQUIRE(loaded.size() == 1);
  CHECK(loaded[0].vault == 0u);
  loaded[0].vault = 2500;
  REQUIRE(db.upsertPledge(loaded[0], &err));
  server::Db db2;
  REQUIRE(db2.open(path, &err));
  std::vector<server::PledgeRec> reloaded;
  REQUIRE(db2.loadPledges(&reloaded, &err));
  REQUIRE(reloaded.size() == 1);
  CHECK(reloaded[0].vault == 2500u);
  rmDb(path);
}

TEST_CASE("T-140: tithe journal parity — kind 41 replays id-identical") {
  auto run = []() {
    server::World w;
    w.loadFrom(makeArena());
    server::Entity* liege = found(w, "liege", "Ashfall");
    server::Command t;
    t.kind = server::Command::kPledgeTithe;
    t.a = 1500;
    server::applyWorldCommand(w, *w.find(liege->id), t);
    return std::pair<std::uint32_t, std::uint32_t>(
        w.find(liege->id)->gold, w.pledgeById(liege->pledgeId)->vault);
  };
  const auto [goldA, vaultA] = run();
  const auto [goldB, vaultB] = run();
  CHECK(goldA == goldB);
  CHECK(vaultA == vaultB);
  CHECK(vaultA == 1500u);
  CHECK(static_cast<int>(server::Command::kPledgeTithe) == 41);
}
