// Named elites (GDD §9): one card per elite (T-101 Maw, T-102 Widow,
// T-103 Cantor). Shared first-blood announce rides the first card.
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

#include "content/mobs.h"
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

bool sawFirstBlood(const server::World& w, const std::string& name) {
  for (const auto& ev : w.events())
    if (ev.chatCh == 2 &&
        ev.chatText.find(name) != std::string::npos &&
        ev.chatText.find("first blood") != std::string::npos)
      return true;
  return false;
}
}  // namespace

TEST_CASE("T-101: Old Maw table law — Gnoll base through the 1010 pattern") {
  const content::MobDef* maw = content::findMob(1012);
  REQUIRE(maw != nullptr);
  CHECK(std::string(maw->name) == "Old Maw");
  CHECK(maw->level == 7);
  CHECK(maw->hp == 145);    // ~0.9x Gnoll 160
  CHECK(maw->dmg == 19);    // +2 over Gnoll 17
  CHECK(maw->def == 12);    // +2 over Gnoll 10
  CHECK(maw->xp == 1200);   // ~4x Gnoll 300 (1010 pattern)
  CHECK(maw->boss == 0);    // no bolt, no telegraph — melee elite
  CHECK(content::namedEliteIdx(1012) == 0);
  CHECK(content::namedEliteIdx(1005) == -1);  // base Gnoll is no elite
  CHECK(content::namedEliteIdx(1013) == 1);   // future cards' slots reserved
  CHECK(content::namedEliteIdx(1014) == 2);
}

TEST_CASE("T-101: Maw pit spawns in fields_overflow (mapgen truth)") {
  server::World w;
  std::string err;
  REQUIRE(w.loadZone(2, "assets/maps/fields_overflow.bhmap", &err));
  const sim::Map* fields = w.zoneMap(2);
  REQUIRE(fields != nullptr);
  CHECK(fields->spawners.size() == 5);  // 4 camps + Maw pit
  bool pit = false;
  for (const auto& sd : fields->spawners)
    if (sd.mobId == 1012) {
      pit = true;
      CHECK(sd.maxAlive == 1);
      CHECK(sd.respawnTicks == 36000);  // 30-min rotation, deterministic
    }
  CHECK(pit);
}

TEST_CASE("T-101: first blood broadcasts once per reboot") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const content::MobDef* maw = content::findMob(1012);
  REQUIRE(maw != nullptr);
  server::Entity* p = spawnP(w, "mawsbane", 10, 10);
  server::Entity& m1 = w.debugSpawnMob(*maw, sim::TilePos{11, 10});
  w.debugKillMob(m1, p);
  CHECK(sawFirstBlood(w, "Old Maw"));
  // second kill, same reboot: purse, no parade
  server::Entity& m2 = w.debugSpawnMob(*maw, sim::TilePos{12, 10});
  w.debugKillMob(m2, p);
  // direct calls don't tick, so events accumulate: exactly one first-blood
  // across both kills proves the session flag (a second announcement would
  // read 2 here).
  std::size_t bloods = 0;
  for (const auto& ev : w.events())
    if (ev.chatCh == 2 && ev.chatText.find("first blood") != std::string::npos)
      ++bloods;
  CHECK(bloods == 1);
  // mob-on-mob elite violence is not a saga: no killer player, no parade
  server::World w2;
  REQUIRE(w2.loadFrom(makeArena()));
  const content::MobDef* rat = content::findMob(1001);
  REQUIRE(rat != nullptr);
  server::Entity& mm = w2.debugSpawnMob(*maw, sim::TilePos{10, 10});
  server::Entity& rr = w2.debugSpawnMob(*rat, sim::TilePos{11, 10});
  w2.debugKillMob(mm, &rr);
  CHECK_FALSE(sawFirstBlood(w2, "Old Maw"));
}

TEST_CASE("T-102: Red Widow table law — Widow base through the 1010 pattern") {
  const content::MobDef* rw = content::findMob(1013);
  REQUIRE(rw != nullptr);
  CHECK(std::string(rw->name) == "Red Widow");
  CHECK(rw->level == 9);
  CHECK(rw->hp == 200);    // ~0.9x Widow 220
  CHECK(rw->dmg == 24);    // +2 over Widow 22
  CHECK(rw->def == 14);    // +2 over Widow 12
  CHECK(rw->xp == 1680);   // ~4x Widow 420 (1010 pattern)
  CHECK(rw->boss == 0);    // melee elite, no bolt
  CHECK(content::namedEliteIdx(1013) == 1);
}

TEST_CASE("T-102: Widow nest spawns in bonehowl_mine (mapgen truth)") {
  server::World w;
  std::string err;
  REQUIRE(w.loadZone(4, "assets/maps/bonehowl_mine.bhmap", &err));
  const sim::Map* mine = w.zoneMap(4);
  REQUIRE(mine != nullptr);
  CHECK(mine->spawners.size() == 8);  // 7 camps + Widow nest
  bool nest = false;
  for (const auto& sd : mine->spawners)
    if (sd.mobId == 1013) {
      nest = true;
      CHECK(sd.maxAlive == 1);
      CHECK(sd.respawnTicks == 54000);  // 45-min rotation, deterministic
    }
  CHECK(nest);
}

TEST_CASE("T-102: Widow first blood reuses the shared announce") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const content::MobDef* rw = content::findMob(1013);
  REQUIRE(rw != nullptr);
  server::Entity* p = spawnP(w, "widowbane", 10, 10);
  server::Entity& m1 = w.debugSpawnMob(*rw, sim::TilePos{11, 10});
  w.debugKillMob(m1, p);
  CHECK(sawFirstBlood(w, "Red Widow"));
  server::Entity& m2 = w.debugSpawnMob(*rw, sim::TilePos{12, 10});
  w.debugKillMob(m2, p);
  std::size_t bloods = 0;
  for (const auto& ev : w.events())
    if (ev.chatCh == 2 && ev.chatText.find("first blood") != std::string::npos)
      ++bloods;
  CHECK(bloods == 1);  // session flag shared with Maw (per-elite slots)
}
