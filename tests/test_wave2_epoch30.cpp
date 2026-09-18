// Wave-2 epoch-30 pins: T-167 creation/sex, T-160 five-stat model,
// T-161 Resurrect rebate, T-162 roster + night premium, T-163 field-war EK,
// T-166 bounty persistence. One file per wave (not per card) to keep the
// suite legible; each TEST_CASE names its card.
#include <cstdint>
#include <filesystem>
#include <string>

#include <doctest/doctest.h>
#include <sqlite3.h>

#include "command.h"
#include "content/kits.h"
#include "content/mobs.h"
#include "persist.h"
#include "sim/clock.h"
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

sim::Tick tickAtHour(float h) {
  const double fromStart = h - bh::sim::kStartHour;
  const double wrapped = fromStart < 0 ? fromStart + 24.0 : fromStart;
  return static_cast<sim::Tick>(wrapped *
                               static_cast<double>(bh::sim::kTicksPerGameHour));
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

TEST_CASE("T-167: fresh rows arrive Unsworn/unknown with the fresh flag") {
  const std::string path = tmpDbPath("bh_wave2_fresh.bhdb");
  rmDb(path);
  server::Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  server::CharacterRow row;
  std::uint8_t reason = 0;
  bool fresh = false;
  REQUIRE(db.loginOrCreate("newblood", "pw123", &row, &reason, &err, &fresh));
  CHECK(fresh);
  CHECK(row.classId == 0);  // Unsworn until the creation panel answers
  CHECK(row.sex == 0);
  // returning login is not fresh
  server::CharacterRow row2;
  REQUIRE(db.loginOrCreate("newblood", "pw123", &row2, &reason, &err, &fresh));
  CHECK_FALSE(fresh);
}

TEST_CASE("T-167: setCreation validates domain, round-trips class+sex") {
  const std::string path = tmpDbPath("bh_wave2_create.bhdb");
  rmDb(path);
  server::Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  server::CharacterRow row;
  std::uint8_t reason = 0;
  REQUIRE(db.loginOrCreate("choosey", "pw123", &row, &reason, &err));
  CHECK_FALSE(db.setCreation(row.id, 0, 1, &err));   // no kit 0
  CHECK_FALSE(db.setCreation(row.id, 4, 1, &err));   // no kit 4
  CHECK_FALSE(db.setCreation(row.id, 2, 0, &err));   // no sex 0
  CHECK_FALSE(db.setCreation(row.id, 2, 3, &err));   // no sex 3
  REQUIRE(db.setCreation(row.id, 2, 2, &err));       // Gravecaller, female
  server::Db db2;
  REQUIRE(db2.open(path, &err));
  server::CharacterRow row2;
  REQUIRE(db2.loginByRowId(row.id, &row2, &reason, &err));
  CHECK(row2.classId == 2);
  CHECK(row2.sex == 2);
}

TEST_CASE("T-167: schema v15 migrates, sex packs into spawn") {
  const std::string path = tmpDbPath("bh_wave2_v15.bhdb");
  rmDb(path);
  server::Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  sqlite3* check = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &check) == SQLITE_OK);
  sqlite3_stmt* st = nullptr;
  REQUIRE(sqlite3_prepare_v2(check, "PRAGMA user_version;", -1, &st, nullptr) ==
          SQLITE_OK);
  REQUIRE(sqlite3_step(st) == SQLITE_ROW);
  CHECK(sqlite3_column_int(st, 0) == 16);  // T-153: v16 (was v15 at wave-2)
  sqlite3_finalize(st);
  sqlite3_close(check);
  // spawn carries sex (T-142 fields finally live)
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& e = w.spawn("sister", 0, sim::TilePos{5, 5}, 1, 2, 2);
  CHECK(e.classId == 2);
  CHECK(e.sex == 2);
}

TEST_CASE("T-160: INT/MAG assignable, domain ends at 4") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "scholar", 5, 5);
  p->statPoints = 3;
  REQUIRE(w.assignStat(*p, 3));
  CHECK(p->intg == 1);
  REQUIRE(w.assignStat(*p, 4));
  CHECK(p->mag == 1);
  CHECK_FALSE(w.assignStat(*p, 5));  // out of domain, point kept
  CHECK(p->statPoints == 1);
}

TEST_CASE("T-160: ch10 unlocks at 20 for Cultist only") {
  CHECK(content::kitSkillUnlock(content::kKitCultist, 10) == 20);
  CHECK(content::kitSkillUnlock(content::kKitRavager, 10) == 0);
  CHECK(content::kitSkillUnlock(content::kKitGravecaller, 10) == 0);
  // T-161b.1: the spare slot is spent — ch11 Sanctuary, Cultist 14 only.
  CHECK(content::kitSkillUnlock(content::kKitCultist, 11) == 14);
  CHECK(content::kitSkillUnlock(content::kKitRavager, 11) == 0);
  CHECK(content::kitSkillUnlock(content::kKitGravecaller, 11) == 0);
}

TEST_CASE("T-161: Resurrect rebates half the nominal debt") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnP(w, "chanter", 5, 5);
  c->classId = content::kKitCultist;
  c->level = 20;
  c->mp = 100;
  server::Entity* v = spawnP(w, "fallen", 6, 5);
  v->level = 5;
  v->xp = 0;
  v->lastDebtXp = 200;
  v->lastDeathTick = w.tickCount();
  w.trySkill(*c, 10, v->id);
  CHECK(w.find(v->id)->xp == 100u);  // half of 200 rebated (may re-level)
  CHECK(w.find(v->id)->lastDebtXp == 0u);  // one rebate per death
  CHECK(c->mp == 100u - 25u);
}

TEST_CASE("T-161: Resurrect refusals (kit/level/range/window/debt/CD)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto mkCultist = [&](const char* n, int x, int y) {
    server::Entity* c = spawnP(w, n, x, y);
    c->classId = content::kKitCultist;
    c->level = 20;
    c->mp = 100;
    return c;
  };
  // non-cultist: silent no-op
  {
    server::Entity* r = spawnP(w, "sword", 5, 5);
    r->classId = content::kKitRavager;
    r->level = 20;
    r->mp = 100;
    server::Entity* v = spawnP(w, "v1", 6, 5);
    v->lastDebtXp = 200;
    v->lastDeathTick = w.tickCount();
    w.trySkill(*r, 10, v->id);
    CHECK(w.find(v->id)->lastDebtXp == 200u);
  }
  // under-level cultist
  {
    server::Entity* c = mkCultist("young", 5, 6);
    c->level = 19;
    server::Entity* v = spawnP(w, "v2", 6, 6);
    v->lastDebtXp = 200;
    v->lastDeathTick = w.tickCount();
    w.trySkill(*c, 10, v->id);
    CHECK(w.find(v->id)->lastDebtXp == 200u);
  }
  // too far (7 > 6)
  {
    server::Entity* c = mkCultist("far", 5, 5);
    server::Entity* v = spawnP(w, "v3", 12, 5);
    v->lastDebtXp = 200;
    v->lastDeathTick = w.tickCount();
    w.trySkill(*c, 10, v->id);
    CHECK(w.find(v->id)->lastDebtXp == 200u);
  }
  // too late (>6000 ticks)
  {
    server::Entity* c = mkCultist("late", 5, 7);
    server::Entity* v = spawnP(w, "v4", 6, 7);
    v->lastDebtXp = 200;
    v->lastDeathTick = w.tickCount() - 6001;
    w.trySkill(*c, 10, v->id);
    CHECK(w.find(v->id)->lastDebtXp == 200u);
  }
  // no debt left
  {
    server::Entity* c = mkCultist("empty", 5, 8);
    server::Entity* v = spawnP(w, "v5", 6, 8);
    v->lastDebtXp = 0;
    v->lastDeathTick = w.tickCount();
    const std::uint32_t mp0 = c->mp;
    w.trySkill(*c, 10, v->id);
    CHECK(c->mp == mp0);  // refused before the toll
  }
  // cooldown: second cast within 6000 ticks refuses
  {
    server::Entity* c = mkCultist("twice", 5, 9);
    server::Entity* v = spawnP(w, "v6", 6, 9);
    v->lastDebtXp = 200;
    v->lastDeathTick = w.tickCount();
    w.trySkill(*c, 10, v->id);
    REQUIRE(w.find(v->id)->lastDebtXp == 0u);
    server::Entity* v7 = spawnP(w, "v7", 6, 9);
    v7->lastDebtXp = 200;
    v7->lastDeathTick = w.tickCount();
    const std::uint32_t mp0 = c->mp;
    w.trySkill(*c, 10, v7->id);
    CHECK(w.find(v7->id)->lastDebtXp == 200u);
    CHECK(c->mp == mp0);
  }
}

TEST_CASE("T-162: D9 rename + new rows resolve") {
  CHECK(std::string(content::findMob(1007)->name) == "Waxen Celebrant");
  const std::uint32_t ids[] = {1015, 1016, 1017, 1018, 1019,
                               1020, 1021, 1022, 1023, 1024, 1025};
  for (const std::uint32_t id : ids) {
    const content::MobDef* md = content::findMob(id);
    REQUIRE(md != nullptr);
    CHECK(std::string(md->name).size() > 0);
    CHECK(md->hp > 0u);
  }
  CHECK(content::findMob(1024)->town == 2u);  // Synod patrol
  CHECK(content::findMob(1025)->town == 1u);  // Ashen patrol
  CHECK(content::findMob(1001)->town == 0u);  // unaffiliated default
}

TEST_CASE("T-162: night premium +50% for night-spawned quarry at night") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto run = [&](bool night, bool flagged) {
    server::World ww;
    ww.loadFrom(makeArena());
    server::Entity* p = spawnP(ww, night ? "owl" : "lark", 5, 5);
    server::Entity* rat =
        ww.find(ww.debugSpawnMob(*content::findMob(1001), {6, 5}).id);
    rat->hp = 1;
    rat->nightSpawned = flagged;
    ww.debugSetTick(tickAtHour(night ? 22.0f : 12.0f));
    const std::uint32_t ratId = rat->id;
    const std::uint32_t pid = p->id;
    server::Command c;
    c.kind = server::Command::kAttack;
    c.a = static_cast<std::int32_t>(ratId);
    server::applyWorldCommand(ww, *ww.find(pid), c);
    int guard = 400;
    while (ww.find(ratId) != nullptr && guard-- > 0) ww.tick();
    REQUIRE(ww.find(ratId) == nullptr);
    return ww.find(pid)->xp;
  };
  // rat base 40: day 40, night 44 (+10%), flagged night 66 (+10% then +50%)
  CHECK(run(false, false) == 40u);
  CHECK(run(true, false) == 44u);
  CHECK(run(true, true) == 66u);
  CHECK(run(false, true) == 40u);  // day-stragglers pay base
}

TEST_CASE("T-163: sworn killers mint EK from enemy patrols, karma-clean") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* sworn = spawnP(w, "ashenblade", 5, 5);
  sworn->townId = 1;  // Thornwall
  server::Entity* patrol =
      w.find(w.debugSpawnMob(*content::findMob(1024), {6, 5}).id);  // town 2
  patrol->hp = 1;
  w.debugKillMob(*patrol, sworn);
  CHECK(w.find(sworn->id)->ek == 1u);
  CHECK(w.find(sworn->id)->karma == 0);  // war is not murder
  // unsworn: no EK
  server::Entity* stray = spawnP(w, "stray", 5, 6);
  server::Entity* patrol2 =
      w.find(w.debugSpawnMob(*content::findMob(1024), {6, 6}).id);
  patrol2->hp = 1;
  w.debugKillMob(*patrol2, stray);
  CHECK(w.find(stray->id)->ek == 0u);
  // same town: no EK
  server::Entity* kin = spawnP(w, "kinslayer", 5, 7);
  kin->townId = 2;
  server::Entity* patrol3 =
      w.find(w.debugSpawnMob(*content::findMob(1024), {6, 7}).id);
  patrol3->hp = 1;
  w.debugKillMob(*patrol3, kin);
  CHECK(w.find(kin->id)->ek == 0u);
}

TEST_CASE("T-166: bounty mark persists, pays once, then drains") {
  const std::string path = tmpDbPath("bh_wave2_bounty.bhdb");
  rmDb(path);
  server::Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  server::CharacterRow row;
  std::uint8_t reason = 0;
  REQUIRE(db.loginOrCreate("marked", "pw123", &row, &reason, &err));
  db.saveProgress(row.id, row.level, row.xp, row.str, row.vit, row.dex,
                  row.statPoints, row.gold, row.invBlob, row.anvilMercy,
                  row.karma, row.classId, row.swordSkill, row.swingLands,
                  row.townId, row.ek, row.pledgeId, row.pledgeRank, row.sex,
                  row.lastDeathTick, row.lastDebtXp, row.lastResTick,
                  1009, 0);  // marked: Gravemother, cycle 0
  server::Db db2;
  REQUIRE(db2.open(path, &err));
  server::CharacterRow row2;
  REQUIRE(db2.loginOrCreate("marked", "pw123", &row2, &reason, &err));
  CHECK(row2.bountyMob == 1009);
  CHECK(row2.bountyCycle == 0);
  // live: cycle-0 quarry is the Gravemother (1500g), one kill drains
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "marked", 5, 5);
  p->bountyMobId = 1009;
  p->bountyCycle = 0;
  const std::uint32_t gold0 = p->gold;
  server::Entity* boss =
      w.find(w.debugSpawnMob(*content::findMob(1009), {6, 5}).id);
  boss->hp = 1;
  w.debugSetTick(tickAtHour(12.0f));  // day: no night math in the assertion
  w.debugKillMob(*boss, p);
  CHECK(w.find(p->id)->gold - gold0 >= 1500u);
  CHECK(w.find(p->id)->bountyMobId == 0u);  // drained, no double-pay
}
