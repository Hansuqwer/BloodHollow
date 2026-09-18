// T-161b.2 Curse of Weakness (chan 12): the Cultist's debility.
// GDD §3 fixes only the -15%; every other number is flagged derivation
// (see world.cpp kWeak* block): unlock 12, MP 12, 8 s hold, 6 s CD,
// Mend range. Enemies only (mobs + non-party players); Purify lifts it
// (one law both ways). Zero RNG by construction.
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
#include "content/kits.h"
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

server::Entity* spawnKit(server::World& w, const char* name, int x, int y,
                         std::uint8_t kit) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y}, 1, kit);
  return w.find(e.id);
}

bool sawKind(const server::World& w, std::uint8_t kind) {
  for (const auto& ev : w.events())
    if (ev.kind == kind) return true;
  return false;
}
}  // namespace

TEST_CASE("T-161b.2: weaken lays -15% dmg/def on a mob") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "witherer", 20, 20, content::kKitCultist);
  c->level = 12;
  c->mp = 30;
  const content::MobDef* ghoul = content::findMob(1002);  // dmg 9, def 5
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{22, 20});
  const std::uint32_t dmg0 = w.effDmgBase(m);
  const std::uint32_t def0 = w.effDef(m);
  REQUIRE(dmg0 == 9);
  REQUIRE(def0 == 5);
  w.trySkill(*c, 12, m.id);
  CHECK(w.find(m.id)->weakUntil > w.tickCount());
  CHECK(w.effDmgBase(*w.find(m.id)) == 9u * 85u / 100u);
  CHECK(w.effDef(*w.find(m.id)) == 5u * 85u / 100u);
  CHECK(w.find(c->id)->mp == 30u - 12u);
  CHECK(sawKind(w, 18));
}

TEST_CASE("T-161b.2: gates — kit, level, MP, CD, range, self, party") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "witherer", 20, 20, content::kKitCultist);
  c->level = 12;
  c->mp = 100;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  // Ravager has no rite on chan 12
  server::Entity* r = spawnKit(w, "sword", 10, 10, content::kKitRavager);
  r->level = 25;
  r->mp = 30;
  server::Entity& m0 = w.debugSpawnMob(*ghoul, sim::TilePos{10, 12});
  w.trySkill(*r, 12, m0.id);
  CHECK(w.find(m0.id)->weakUntil == -1);
  // below unlock 12: nothing
  server::Entity* y = spawnKit(w, "young", 20, 22, content::kKitCultist);
  y->level = 11;
  y->mp = 30;
  server::Entity& m1 = w.debugSpawnMob(*ghoul, sim::TilePos{20, 24});
  w.trySkill(*y, 12, m1.id);
  CHECK(w.find(m1.id)->weakUntil == -1);
  // broke: quiet fail
  server::Entity* b = spawnKit(w, "spent", 20, 20, content::kKitCultist);
  b->level = 12;
  b->mp = 11;
  server::Entity& m2 = w.debugSpawnMob(*ghoul, sim::TilePos{21, 20});
  w.trySkill(*b, 12, m2.id);
  CHECK(w.find(m2.id)->weakUntil == -1);
  // out of reach (d=8 > 6): quiet fail
  server::Entity& m3 = w.debugSpawnMob(*ghoul, sim::TilePos{20, 28});
  w.trySkill(*c, 12, m3.id);
  CHECK(w.find(m3.id)->weakUntil == -1);
  // self refused
  w.trySkill(*c, 12, c->id);
  CHECK(w.find(c->id)->weakUntil == -1);
  // party mate refused (no choir-on-choir griefing)
  server::Entity* mate = spawnKit(w, "mate", 21, 20, content::kKitCultist);
  REQUIRE(w.partyInvite(*c, *mate));
  REQUIRE(w.partyAccept(*w.find(mate->id)));
  w.trySkill(*w.find(c->id), 12, mate->id);
  CHECK(w.find(mate->id)->weakUntil == -1);
  // stranger (non-party player) takes it
  server::Entity* foe = spawnKit(w, "foe", 22, 20, content::kKitRavager);
  const std::uint32_t foe0 = w.effDmgBase(*foe);
  w.trySkill(*w.find(c->id), 12, foe->id);
  CHECK(w.find(foe->id)->weakUntil > w.tickCount());
  CHECK(w.effDmgBase(*w.find(foe->id)) == foe0 * 85u / 100u);
  // inside the 120t CD the second cast is refused
  server::Entity& m4 = w.debugSpawnMob(*ghoul, sim::TilePos{19, 20});
  w.trySkill(*w.find(c->id), 12, m4.id);
  CHECK(w.find(m4.id)->weakUntil == -1);
}

TEST_CASE("T-161b.2: hold lets go at 160t") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "witherer", 20, 20, content::kKitCultist);
  c->level = 12;
  c->mp = 30;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{22, 20});
  w.trySkill(*c, 12, m.id);
  REQUIRE(w.effDmgBase(*w.find(m.id)) == 9u * 85u / 100u);
  for (int i = 0; i < 160; ++i) w.tick();
  CHECK(w.effDmgBase(*w.find(m.id)) == 9u);
  CHECK(w.effDef(*w.find(m.id)) == 5u);
}

TEST_CASE("T-161b.2: Purify lifts the laid weakness (one law both ways)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "witherer", 20, 20, content::kKitCultist);
  c->level = 12;
  c->mp = 60;
  server::Entity* mate = spawnKit(w, "mate", 21, 20, content::kKitCultist);
  mate->level = 6;
  mate->mp = 30;
  REQUIRE(w.partyInvite(*c, *mate));
  REQUIRE(w.partyAccept(*w.find(mate->id)));
  // an enemy witherer lays it on the mate
  server::Entity* foe = spawnKit(w, "foe", 24, 20, content::kKitCultist);
  foe->level = 12;
  foe->mp = 30;
  w.trySkill(*foe, 12, mate->id);
  REQUIRE(w.find(mate->id)->weakUntil > w.tickCount());
  // own choir purges it (mate is level 6: Purify lane, not Weaken lane)
  w.trySkill(*w.find(mate->id), 9, mate->id);
  CHECK(w.find(mate->id)->weakUntil == -1);
}

TEST_CASE("T-161b.2: chan 12 through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "relayed", 20, 20, content::kKitCultist);
  c->level = 12;
  c->mp = 30;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{22, 20});
  server::Command cmd;
  cmd.kind = server::Command::kSkill;
  cmd.channel = 12;
  cmd.a = static_cast<std::int32_t>(m.id);
  server::applyWorldCommand(w, *c, cmd);
  CHECK(w.find(m.id)->weakUntil > w.tickCount());
}
