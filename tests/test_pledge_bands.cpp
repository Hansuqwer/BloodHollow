// T-139 pledge bands: sworn captains muster the sworn war-host (one pledge,
// one band) instead of party-shaped bands.
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

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

bool enlisted(const server::World& w, std::uint32_t id) {
  for (const std::uint32_t e : w.siegeAttackers())
    if (e == id) return true;
  return false;
}
}  // namespace

TEST_CASE("T-139: sworn captain musters the whole sworn war-host as one band") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* liege = spawnP(w, "liege", 5, 5);
  server::Entity* sworn2 = spawnP(w, "sworn2", 6, 5);
  server::Entity* sworn3 = spawnP(w, "sworn3", 7, 5);
  server::Entity* fallen = spawnP(w, "fallen", 8, 5);
  server::Entity* stray = spawnP(w, "stray", 9, 5);
  liege->pledgeId = 7;
  sworn2->pledgeId = 7;
  sworn3->pledgeId = 7;
  fallen->pledgeId = 7;
  fallen->dead = true;  // the dead march with no band
  // stray stays unaffiliated

  CHECK(w.siegeRegister(liege->id));
  CHECK(w.siegeAttackers().size() == 3u);  // liege + sworn2 + sworn3
  CHECK(enlisted(w, liege->id));
  CHECK(enlisted(w, sworn2->id));
  CHECK(enlisted(w, sworn3->id));
  CHECK_FALSE(enlisted(w, fallen->id));
  CHECK_FALSE(enlisted(w, stray->id));
  CHECK_FALSE(w.siegeRegister(liege->id));  // pure dup stays quiet
}

TEST_CASE("T-139: one pledge one band — late members muster, no second band") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* liege = spawnP(w, "liege", 5, 5);
  server::Entity* late = spawnP(w, "late", 6, 5);
  liege->pledgeId = 7;
  late->pledgeId = 7;
  late->dead = true;
  CHECK(w.siegeRegister(liege->id));
  CHECK(w.siegeAttackers().size() == 1u);
  late->dead = false;  // revive past the muster
  CHECK(w.siegeRegister(late->id));  // musters into the existing band
  CHECK(w.siegeAttackers().size() == 2u);
  CHECK(enlisted(w, late->id));
  // unrelated pledge still opens its own band
  server::Entity* rival = spawnP(w, "rival", 10, 5);
  rival->pledgeId = 8;
  CHECK(w.siegeRegister(rival->id));
  CHECK(w.siegeAttackers().size() == 3u);
}

TEST_CASE("T-139: the 8-band cap counts pledge bands") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* liege = spawnP(w, "liege", 5, 5);
  server::Entity* sworn2 = spawnP(w, "sworn2", 6, 5);
  liege->pledgeId = 7;
  sworn2->pledgeId = 7;
  CHECK(w.siegeRegister(liege->id));  // band 1 (2 members)
  for (int i = 0; i < 7; ++i) {
    server::Entity* p = spawnP(w, ("solo" + std::to_string(i)).c_str(), 10 + i, 10);
    REQUIRE(w.siegeRegister(p->id));  // bands 2..8
  }
  CHECK(w.siegeAttackers().size() == 9u);  // 2 sworn + 7 solo
  server::Entity* ninth = spawnP(w, "ninth", 30, 30);
  CHECK_FALSE(w.siegeRegister(ninth->id));  // camp is full
  server::Entity* swornLate = spawnP(w, "swornlate", 31, 30);
  swornLate->pledgeId = 7;
  // pledge 7 already enlisted: muster path needs no free band slot
  CHECK(w.siegeRegister(swornLate->id));
  CHECK(enlisted(w, swornLate->id));
}

TEST_CASE("T-139: desertion never un-enlists — the muster is session law") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* liege = spawnP(w, "liege", 5, 5);
  server::Entity* deserter = spawnP(w, "deserter", 6, 5);
  liege->pledgeId = 7;
  deserter->pledgeId = 7;
  CHECK(w.siegeRegister(liege->id));
  CHECK(w.siegeAttackers().size() == 2u);
  deserter->pledgeId = 0;  // walks from the oath mid-session
  CHECK(enlisted(w, deserter->id));  // still rides the band
  CHECK_FALSE(w.siegeRegister(deserter->id));  // unaffiliated now, already in camp: quiet dup
}
