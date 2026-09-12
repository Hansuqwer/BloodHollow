// T-122 pledge-lite, pinned at World-API level.
// Semantics locked (05-mvp.md cut): level >= 10 + 10,000g to found (CHA-gap
// stand-in, flagged in the card), registrar proximity Chebyshev <= 3 (confessor
// pattern), name 3..16 of [A-Za-z0-9_-] unique forever, ranks Liege(3) /
// Bloodsworn(2) / Initiate(1), Liege+Bloodsworn invite at <= 12 tiles,
// accepts join as Initiate, invites expire after 400 ticks, kick/promote/
// demote are Liege-only, liege leave == disband, cap 20 members, membership
// persists by NAME (registry of record) while the entity cache is id+rank.
// Journal invariant: pledge commands as id-resolved Commands replay to
// identical membership state; g-sidecar restore synthesizes registry stubs.
#include <doctest/doctest.h>

#include <cstdint>
#include <filesystem>
#include <string>

#include <sqlite3.h>

#include "command.h"
#include "persist.h"
#include "sim/combat.h"
#include "world.h"

using namespace bh;

namespace {
sim::Map makeArena() {
  sim::Map m;
  m.w = 60;
  m.h = 60;
  m.tileW = 64;
  m.tileH = 32;
  m.ground.assign(60 * 60, 0);
  m.zone.assign(60 * 60, 0);
  m.blocked.assign(60 * 60, 0);
  return m;  // no spawners: tests drive pledge APIs directly
}

server::Entity* spawnP(server::World& w, const char* name, int x, int y) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y});
  return w.find(e.id);
}

// The registrar stands three tiles east (inside the radius) unless told.
void registrarNear(server::World& w, int x = 13, int y = 10) {
  (void)w.debugSpawnRegistrar(sim::TilePos{x, y});
}

void makeFounder(server::Entity* p) {
  p->level = 10;
  p->gold = 20000;
}

std::string tmpDbPath(const char* tag) {
  return (std::filesystem::temp_directory_path() /
          (std::string("bh_pledge_") + tag + ".bhdb"))
      .string();
}

void rmDb(const std::string& p) {
  std::filesystem::remove(p);
  std::filesystem::remove(p + "-wal");
  std::filesystem::remove(p + "-shm");
}
}  // namespace

TEST_CASE("T-122: founding — gates, toll, Liege rank") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "founder", 10, 10);
  registrarNear(w);

  SUBCASE("too green") {
    p->level = 9;
    p->gold = 20000;
    CHECK_FALSE(w.pledgeCreate(*p, "FirstBlood"));
  }
  SUBCASE("too poor") {
    p->level = 10;
    p->gold = 9999;
    CHECK_FALSE(w.pledgeCreate(*p, "FirstBlood"));
  }
  SUBCASE("name shape") {
    makeFounder(p);
    CHECK_FALSE(w.pledgeCreate(*p, "ab"));           // < 3
    CHECK_FALSE(w.pledgeCreate(*p, "way_too_long_for_a_name"));  // > 16
    CHECK_FALSE(w.pledgeCreate(*p, "has space"));    // charset
    CHECK_FALSE(w.pledgeCreate(*p, "spicy!"));       // charset
  }
  SUBCASE("no registrar nearby") {
    server::World w2;
    REQUIRE(w2.loadFrom(makeArena()));
    server::Entity* q = spawnP(w2, "farmer", 10, 10);
    makeFounder(q);
    (void)w2.debugSpawnRegistrar(sim::TilePos{40, 40});
    CHECK_FALSE(w2.pledgeCreate(*q, "FirstBlood"));
  }
  SUBCASE("success — toll paid, Liege sworn, registered") {
    makeFounder(p);
    REQUIRE(w.pledgeCreate(*p, "FirstBlood"));
    CHECK(p->gold == 10000);  // 20000 - 10000
    CHECK(p->pledgeRank == 3);
    CHECK(p->pledgeId == 1);
    REQUIRE(w.pledgeById(1) != nullptr);
    CHECK(w.pledgeById(1)->name == "FirstBlood");
    CHECK(w.pledgeById(1)->liege == "founder");
    REQUIRE(w.pledgeById(1)->members.size() == 1);
    CHECK(w.pledgeById(1)->members[0] == "founder");
    CHECK(w.pledgesDirty);
    // names are unique forever
    server::Entity* r = spawnP(w, "rival", 11, 10);
    makeFounder(r);
    CHECK_FALSE(w.pledgeCreate(*r, "FirstBlood"));
    // and no double-founding while sworn
    CHECK_FALSE(w.pledgeCreate(*p, "SecondBlood"));
  }
}

TEST_CASE("T-122: registrar proximity — Chebyshev 3 exact edge") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  (void)w.debugSpawnRegistrar(sim::TilePos{10, 10});
  server::Entity* a = spawnP(w, "inside", 13, 13);  // exactly 3 diag
  server::Entity* b = spawnP(w, "outside", 14, 13);  // 4 east
  makeFounder(a);
  makeFounder(b);
  CHECK(w.nearRegistrar(*a));
  CHECK_FALSE(w.nearRegistrar(*b));
  CHECK(w.pledgeCreate(*a, "EdgeCase"));
  CHECK_FALSE(w.pledgeCreate(*b, "AlsoRan"));
}

TEST_CASE("T-122: invite — ranks, range, expiry, accept-as-Initiate") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* liege = spawnP(w, "liege", 10, 10);
  registrarNear(w);
  makeFounder(liege);
  REQUIRE(w.pledgeCreate(*liege, "Oathkeepers"));

  SUBCASE("accept joins as Initiate, registry by name") {
    server::Entity* fresh = spawnP(w, "fresh", 11, 10);
    REQUIRE(w.pledgeInvite(*liege, *fresh));
    REQUIRE(w.pledgeAccept(*fresh));
    CHECK(fresh->pledgeId == liege->pledgeId);
    CHECK(fresh->pledgeRank == 1);
    REQUIRE(w.pledgeById(liege->pledgeId)->members.size() == 2);
  }
  SUBCASE("invite expires after 400 ticks") {
    server::Entity* slow = spawnP(w, "slow", 11, 10);
    REQUIRE(w.pledgeInvite(*liege, *slow));
    for (int i = 0; i < 401; ++i) w.tick();
    CHECK_FALSE(w.pledgeAccept(*slow));
    CHECK(slow->pledgeId == 0);
  }
  SUBCASE("range: 12 tiles Chebyshev") {
    server::Entity* far = spawnP(w, "far", 22, 10);  // 12 east: in range
    server::Entity* way = spawnP(w, "way", 23, 10);  // 13 east: refused
    CHECK(w.pledgeInvite(*liege, *far));
    CHECK_FALSE(w.pledgeInvite(*liege, *way));
  }
  SUBCASE("Initiates cannot invite; Bloodsworn can") {
    server::Entity* init = spawnP(w, "init", 11, 10);
    server::Entity* other = spawnP(w, "other", 12, 10);
    REQUIRE(w.pledgeInvite(*liege, *init));
    REQUIRE(w.pledgeAccept(*init));
    CHECK(init->pledgeRank == 1);
    CHECK_FALSE(w.pledgeInvite(*init, *other));
    REQUIRE(w.pledgeSetRank(*liege, init->id, 2));  // promote to Bloodsworn
    CHECK(w.pledgeInvite(*init, *other));
  }
  SUBCASE("already-sworn targets are refused") {
    server::Entity* init = spawnP(w, "init", 11, 10);
    REQUIRE(w.pledgeInvite(*liege, *init));
    REQUIRE(w.pledgeAccept(*init));
    CHECK_FALSE(w.pledgeInvite(*liege, *init));
  }
  SUBCASE("member cap 20") {
    for (int i = 0; i < 19; ++i) {
      server::Entity* m = spawnP(w, ("m" + std::to_string(i)).c_str(), 11, 10);
      REQUIRE(w.pledgeInvite(*liege, *m));
      REQUIRE(w.pledgeAccept(*m));
    }
    REQUIRE(w.pledgeById(liege->pledgeId)->members.size() == 20);
    server::Entity* full = spawnP(w, "full", 11, 10);
    CHECK_FALSE(w.pledgeInvite(*liege, *full));
  }
}

TEST_CASE("T-122: rank law — Liege-only promote/demote, no Liege-making") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* liege = spawnP(w, "liege", 10, 10);
  registrarNear(w);
  makeFounder(liege);
  REQUIRE(w.pledgeCreate(*liege, "Hollow"));
  server::Entity* m = spawnP(w, "member", 11, 10);
  REQUIRE(w.pledgeInvite(*liege, *m));
  REQUIRE(w.pledgeAccept(*m));

  SUBCASE("non-liege cannot move ranks") {
    CHECK_FALSE(w.pledgeSetRank(*m, m->id, 2));
  }
  SUBCASE("liege promotes to Bloodsworn and back") {
    REQUIRE(w.pledgeSetRank(*liege, m->id, 2));
    CHECK(m->pledgeRank == 2);
    REQUIRE(w.pledgeSetRank(*liege, m->id, 1));
    CHECK(m->pledgeRank == 1);
  }
  SUBCASE("rank 3 (Liege) is not grantable — transfers are post-lite") {
    CHECK_FALSE(w.pledgeSetRank(*liege, m->id, 3));
    CHECK_FALSE(w.pledgeSetRank(*liege, m->id, 0));
  }
}

TEST_CASE("T-122: leave / kick / disband") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* liege = spawnP(w, "liege", 10, 10);
  registrarNear(w);
  makeFounder(liege);
  REQUIRE(w.pledgeCreate(*liege, "Ashfall"));
  server::Entity* m = spawnP(w, "member", 11, 10);
  REQUIRE(w.pledgeInvite(*liege, *m));
  REQUIRE(w.pledgeAccept(*m));

  SUBCASE("member leaves: registry shrinks, cache cleared") {
    REQUIRE(w.pledgeLeave(*m));
    CHECK(m->pledgeId == 0);
    CHECK(m->pledgeRank == 0);
    REQUIRE(w.pledgeById(liege->pledgeId)->members.size() == 1);
  }
  SUBCASE("kick: Liege-only") {
    server::Entity* m2 = spawnP(w, "member2", 12, 10);
    REQUIRE(w.pledgeInvite(*liege, *m2));
    REQUIRE(w.pledgeAccept(*m2));
    CHECK_FALSE(w.pledgeKick(*m2, m->id));  // Initiate cannot kick
    REQUIRE(w.pledgeKick(*liege, m->id));
    CHECK(m->pledgeId == 0);
    REQUIRE(w.pledgeById(liege->pledgeId)->members.size() == 2);  // liege+m2
  }
  SUBCASE("liege leaving disbands the pledge") {
    REQUIRE(w.pledgeLeave(*liege));  // routes to disband
    CHECK(liege->pledgeId == 0);
    CHECK(m->pledgeId == 0);         // everyone cast out
    CHECK(w.pledgeById(1) == nullptr);
    CHECK(w.pledges().empty());
  }
  SUBCASE("explicit disband clears all caches") {
    REQUIRE(w.pledgeDisband(*liege));
    CHECK(liege->pledgeId == 0);
    CHECK(m->pledgeId == 0);
    CHECK(w.pledges().empty());
    CHECK(w.pledgesDirty);
  }
}

TEST_CASE("T-122: pledge chat reaches online members only") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* liege = spawnP(w, "liege", 10, 10);
  registrarNear(w);
  makeFounder(liege);
  REQUIRE(w.pledgeCreate(*liege, "Whisper"));
  server::Entity* m = spawnP(w, "member", 11, 10);
  server::Entity* stranger = spawnP(w, "stranger", 12, 10);
  REQUIRE(w.pledgeInvite(*liege, *m));
  REQUIRE(w.pledgeAccept(*m));

  const std::size_t before = w.events().size();
  w.pledgeChat(*liege, "hold the gate");
  std::size_t forLiege = 0, forMember = 0, forStranger = 0;
  for (std::size_t i = before; i < w.events().size(); ++i) {
    const auto& ev = w.events()[i];
    REQUIRE(ev.chatCh == 255);
    CHECK(ev.chatText.find("[Whisper] liege: hold the gate") == 0);
    if (ev.aboutId == liege->id) ++forLiege;
    else if (ev.aboutId == m->id) ++forMember;
    else if (ev.aboutId == stranger->id) ++forStranger;
  }
  CHECK(forLiege == 1);
  CHECK(forMember == 1);
  CHECK(forStranger == 0);
  // unsworn cannot speak into the channel
  const std::size_t before2 = w.events().size();
  w.pledgeChat(*stranger, "let me in");
  CHECK(w.events().size() == before2);
}

TEST_CASE("T-122: journal command parity — replay path is id-identical") {
  // Live and replay both funnel through applyWorldCommand; the only live-only
  // datum is the pledge NAME (Command::text), which the c-line drops. Replay
  // synthesizes "pledge-<id>" — hash-neutral: pledge state is not in
  // worldHash by design.
  server::World live;
  REQUIRE(live.loadFrom(makeArena()));
  server::Entity* L = spawnP(live, "liege", 10, 10);
  registrarNear(live);
  makeFounder(L);
  server::Command create;
  create.kind = server::Command::kPledgeCreate;
  create.text = "NamedLive";
  applyWorldCommand(live, *L, create);
  CHECK(L->pledgeId == 1);
  CHECK(live.pledgeById(1)->name == "NamedLive");

  server::Entity* M = spawnP(live, "member", 11, 10);
  server::Command inv;
  inv.kind = server::Command::kPledgeInvite;
  inv.a = static_cast<std::int32_t>(M->id);
  applyWorldCommand(live, *L, inv);
  server::Command acc;
  acc.kind = server::Command::kPledgeAccept;
  applyWorldCommand(live, *M, acc);
  CHECK(M->pledgeRank == 1);

  server::World replay;  // same command stream, textless create
  REQUIRE(replay.loadFrom(makeArena()));
  server::Entity* L2 = spawnP(replay, "liege", 10, 10);
  registrarNear(replay);
  makeFounder(L2);
  server::Command createR;
  createR.kind = server::Command::kPledgeCreate;  // no text: journal replay
  applyWorldCommand(replay, *L2, createR);
  server::Entity* M2 = spawnP(replay, "member", 11, 10);
  server::Command invR;
  invR.kind = server::Command::kPledgeInvite;
  invR.a = static_cast<std::int32_t>(M2->id);
  applyWorldCommand(replay, *L2, invR);
  server::Command accR;
  accR.kind = server::Command::kPledgeAccept;
  applyWorldCommand(replay, *M2, accR);

  CHECK(L2->pledgeId == L->pledgeId);
  CHECK(L2->gold == L->gold);          // toll identical without the name
  CHECK(M2->pledgeId == M->pledgeId);
  CHECK(M2->pledgeRank == M->pledgeRank);
  CHECK(replay.pledgeById(1)->name == "pledge-1");  // synthesized
  CHECK(replay.pledgeById(1)->members == live.pledgeById(1)->members);
}

TEST_CASE("T-122: g-sidecar restore — stubs, membership dedup") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "alpha", 10, 10);
  server::Entity* b = spawnP(w, "beta", 11, 10);
  server::Entity* c = spawnP(w, "gamma", 12, 10);

  // two logins into pledge 5 created in a PRIOR session
  w.pledgeReplayRestore(*a, 5, 3);
  CHECK(a->pledgeId == 5);
  CHECK(a->pledgeRank == 3);
  REQUIRE(w.pledgeById(5) != nullptr);
  CHECK(w.pledgeById(5)->name == "pledge-5");  // DB-of-record name
  CHECK(w.pledgeById(5)->members.size() == 1);
  CHECK(w.nextPledgeId() == 6);

  w.pledgeReplayRestore(*b, 5, 1);
  CHECK(w.pledgeById(5)->members.size() == 2);
  // re-login of the same name never duplicates
  w.pledgeReplayRestore(*a, 5, 3);
  CHECK(w.pledgeById(5)->members.size() == 2);
  // unsworn sidecar is a no-op
  w.pledgeReplayRestore(*c, 0, 0);
  CHECK(c->pledgeId == 0);
  // stub id seeds the allocator: next founding cannot collide
  server::Entity* L = spawnP(w, "founder", 13, 10);
  registrarNear(w, 14, 10);
  makeFounder(L);
  REQUIRE(w.pledgeCreate(*L, "Fresh"));
  CHECK(L->pledgeId == 6);
}

TEST_CASE("T-122: schema v12 — migration, registry + membership round-trip") {
  const std::string path = tmpDbPath("v12");
  rmDb(path);

  // v11 database (post T-118): no pledge columns, no pledges table
  sqlite3* raw = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &raw) == SQLITE_OK);
  char* msg = nullptr;
  REQUIRE(sqlite3_exec(raw,
    "CREATE TABLE IF NOT EXISTS accounts ("
    "  id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE COLLATE NOCASE,"
    "  salt INTEGER NOT NULL, pwhash INTEGER NOT NULL,"
    "  created INTEGER NOT NULL DEFAULT (strftime('%s','now')));"
    "CREATE TABLE IF NOT EXISTS characters ("
    "  id INTEGER PRIMARY KEY, account_id INTEGER NOT NULL REFERENCES accounts(id),"
    "  name TEXT NOT NULL, map_id INTEGER NOT NULL DEFAULT 1,"
    "  x INTEGER NOT NULL DEFAULT 0, y INTEGER NOT NULL DEFAULT 0,"
    "  level INTEGER NOT NULL DEFAULT 1, xp INTEGER NOT NULL DEFAULT 0,"
    "  created INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
    "  str INTEGER NOT NULL DEFAULT 8, vit INTEGER NOT NULL DEFAULT 8,"
    "  dex INTEGER NOT NULL DEFAULT 8, stat_points INTEGER NOT NULL DEFAULT 0,"
    "  gold INTEGER NOT NULL DEFAULT 50, inv TEXT NOT NULL DEFAULT '',"
    "  anvil_mercy INTEGER NOT NULL DEFAULT 0, karma INTEGER NOT NULL DEFAULT 0,"
    "  class_id INTEGER NOT NULL DEFAULT 1,"
    "  sword_skill INTEGER NOT NULL DEFAULT 0,"
    "  swing_lands INTEGER NOT NULL DEFAULT 0);"
    "PRAGMA user_version=11;",
    nullptr, nullptr, &msg) == SQLITE_OK);
  sqlite3_close(raw);

  server::Db db;
  std::string err;
  REQUIRE(db.open(path, &err));  // migrates to v12

  // registry round-trip
  server::PledgeRec rec;
  rec.id = 7;
  rec.name = "Ashfall";
  rec.emblem = 3;
  rec.liege = "liege";
  REQUIRE(db.upsertPledge(rec, &err));
  server::PledgeRec rec2;
  rec2.id = 8;
  rec2.name = "Edgewise";
  rec2.emblem = 9;
  rec2.liege = "edge";
  REQUIRE(db.upsertPledge(rec2, &err));

  std::vector<server::PledgeRec> loaded;
  REQUIRE(db.loadPledges(&loaded, &err));
  REQUIRE(loaded.size() == 2);
  CHECK(loaded[0].name == "Ashfall");
  CHECK(loaded[1].emblem == 9);

  // membership rides saveProgress (pledge_id/pledge_rank columns)
  server::CharacterRow row;
  std::uint8_t fail = 0;
  REQUIRE(db.loginOrCreate("liege", "pw", &row, &fail, &err));
  db.saveProgress(row.id, 12, 0, 8, 8, 8, 0, 100, "", 0, 0, 1, 0, 0, 7, 3);
  REQUIRE(db.loginOrCreate("serf", "pw", &row, &fail, &err));
  db.saveProgress(row.id, 10, 0, 8, 8, 8, 0, 100, "", 0, 0, 1, 0, 0, 7, 1);

  std::vector<std::pair<std::string, std::pair<int, int>>> mems;
  REQUIRE(db.loadPledgeMembers(&mems, &err));
  REQUIRE(mems.size() == 2);
  CHECK(mems[0].first == "liege");
  CHECK(mems[0].second.first == 7);
  CHECK(mems[0].second.second == 3);
  CHECK(mems[1].second.second == 1);

  // login restore carries membership (schema v12 SELECT columns)
  server::CharacterRow again;
  REQUIRE(db.loginOrCreate("liege", "pw", &again, &fail, &err));
  CHECK(again.pledgeId == 7);
  CHECK(again.pledgeRank == 3);

  // disband deletes
  REQUIRE(db.deletePledge(8, &err));
  loaded.clear();
  REQUIRE(db.loadPledges(&loaded, &err));
  REQUIRE(loaded.size() == 1);

  // migration stamp
  sqlite3* check = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &check) == SQLITE_OK);
  sqlite3_stmt* st = nullptr;
  REQUIRE(sqlite3_prepare_v2(check, "PRAGMA user_version;", -1, &st, nullptr) == SQLITE_OK);
  REQUIRE(sqlite3_step(st) == SQLITE_ROW);
  CHECK(sqlite3_column_int(st, 0) == 12);
  sqlite3_finalize(st);
  sqlite3_close(check);
  rmDb(path);
}
