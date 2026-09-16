// T-129 Blood Moon flag + two levers (H4 night war, 1/3).
// Pins: raise-to-dawn, fiction, quiet repeat/dead, tick expiry, lever
// values both states, journaled path, paired-worlds bite ratio.
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

#include "command.h"
#include "content/mobs.h"
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
}  // namespace

TEST_CASE("T-129: gm blood-moon raises it to next dawn with fiction") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));  // tick 0 == 08:00
  server::Entity* p = spawnP(w, "mooncaller", 5, 5);
  CHECK_FALSE(w.bloodMoonActive());
  REQUIRE(w.bloodMoon(*p));
  CHECK(w.bloodMoonActive());
  // 08:00 -> next 05:00 == 21 game-hours
  CHECK(w.bloodMoonUntil() == 21 * bh::sim::kTicksPerGameHour);
  bool herald = false;
  for (const auto& ev : w.events()) {
    if (ev.chatCh == 2 &&
        ev.chatText.find("moon rises red") != std::string::npos)
      herald = true;
  }
  CHECK(herald);
}

TEST_CASE("T-129: one moon at a time, the dead raise nothing") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "first", 5, 5);
  server::Entity* q = spawnP(w, "second", 8, 8);
  REQUIRE(w.bloodMoon(*p));
  const sim::Tick until = w.bloodMoonUntil();
  CHECK_FALSE(w.bloodMoon(*q));  // repeat while red: quiet
  CHECK(w.bloodMoonUntil() == until);
  q->dead = true;
  CHECK_FALSE(w.bloodMoon(*q));
  p->dead = true;
  CHECK_FALSE(w.bloodMoon(*p));
}

TEST_CASE("T-129: the moon sets by tick") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "watcher", 5, 5);
  REQUIRE(w.bloodMoon(*p));
  REQUIRE(w.bloodMoonActive());
  w.debugSetTick(w.bloodMoonUntil());
  w.tick();
  CHECK_FALSE(w.bloodMoonActive());
}

TEST_CASE("T-129: levers follow the flag") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "levers", 5, 5);
  CHECK(w.curseDuration() == 600);
  CHECK(w.nightBiteNum() == 115u);
  REQUIRE(w.bloodMoon(*p));
  CHECK(w.curseDuration() == 1200);
  CHECK(w.nightBiteNum() == 130u);
}

TEST_CASE("T-129: journaled path raises the moon (replay-exact)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "roundtrip", 5, 5);
  server::Command c;
  c.kind = server::Command::kBloodMoon;
  server::applyWorldCommand(w, *p, c);
  CHECK(w.bloodMoonActive());
}

TEST_CASE("T-129: paired worlds bite ~130/115 under the moon") {
  // Seed-identical worlds, night, naked 2000-hp bystander + one ghoul (1002,
  // aggro 6 — the rat is passive); the ONLY difference is the moon. Same
  // stream => same swings; only the multiplier differs (no new draws
  // anywhere on the moon path).
  auto run = [](bool moon) {
    server::World w;
    w.loadFrom(makeArena());
    w.debugSetTick(tickAtHour(22.0f));
    server::Entity* p = spawnP(w, moon ? "red" : "plain", 10, 10);
    p->hpMax = 2000;
    p->hp = 2000;
    w.debugSpawnMob(*content::findMob(1002), sim::TilePos{11, 10});
    if (moon) {
      REQUIRE(w.bloodMoon(*p));
    }
    for (int t = 0; t < 600; ++t) w.tick();
    return 2000u - w.find(p->id)->hp;
  };
  const std::uint32_t base = run(false);
  const std::uint32_t red = run(true);
  REQUIRE(base > 30u);  // the rat actually worked both shifts
  CHECK(red * 100u >= base * 110u);
  CHECK(red * 100u <= base * 116u);
}
