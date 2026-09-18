// T-159f1.5 drop log: one line per awarded drop (gear/unique/junk/gold).
// Pinned line shape (tick, mob, killer, what, detail). The path override
// keeps the suite off the real logs/drops.log (T-080 pattern).
#include <doctest/doctest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
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

std::string tmpLog() {
  return (std::filesystem::temp_directory_path() / "t159f1_drops.log").string();
}

std::string readAll(const std::string& path) {
  std::ifstream f(path);
  if (!f) return "";
  return std::string((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
}

server::Entity* spawnP(server::World& w, const char* name, int x, int y) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y});
  return w.find(e.id);
}
}  // namespace

TEST_CASE("T-159f1.5: a paid kill appends pinned drop lines") {
  const std::string path = tmpLog();
  std::remove(path.c_str());
  server::World::setDropLogPath(path);
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "slayer", 5, 5);
  a->classId = 1;  // Ravager: Power Swing lane
  a->level = 10;
  a->hp = 1000000;
  a->hpMax = 1000000;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{6, 5});
  const std::uint32_t mId = m.id;
  m.hp = 1;
  // swing until it falls (40t CD outwaited; the slayer cannot die)
  for (int i = 0; i < 200 && w.find(mId) != nullptr; ++i) {
    w.trySkill(*w.find(a->id), 1, mId);
    for (int t = 0; t < 40; ++t) w.tick();
  }
  REQUIRE(w.find(mId) == nullptr);
  const std::string log = readAll(path);
  // ghoul gold (14-30) always pays: the gold line is unconditional
  CHECK(log.find("mob=1002") != std::string::npos);
  CHECK(log.find("killer=slayer") != std::string::npos);
  CHECK(log.find("what=gold amount=") != std::string::npos);
}
