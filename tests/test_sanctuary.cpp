// T-161b.1 Sanctuary (chan 11): the Cultist's ground hold.
// GDD §3 fixes only the 600t CD; every other number is flagged derivation
// (see world.cpp kSanct* block): unlock 14, MP 20, radius 3, 600t hold,
// 40t pulses of (12+L)/2, Mend-mirror curse math. Zero RNG by construction.
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
#include "content/kits.h"
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

TEST_CASE("T-161b.1: cast hallows ground (circle, MP, kind-17 signal)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnKit(w, "holder", 20, 20, content::kKitCultist);
  p->level = 14;
  p->mp = 30;
  w.trySkill(*p, 11, p->id);
  CHECK(w.sanctCount() == 1);
  CHECK(w.find(p->id)->mp == 30u - 20u);
  CHECK(sawKind(w, 17));
}

TEST_CASE("T-161b.1: gates — kit, level, MP, CD") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  // Ravager has no rite on chan 11
  server::Entity* r = spawnKit(w, "sword", 20, 20, content::kKitRavager);
  r->level = 25;
  r->mp = 30;
  w.trySkill(*r, 11, r->id);
  CHECK(w.sanctCount() == 0);
  // Cultist below unlock 14: nothing
  server::Entity* y = spawnKit(w, "young", 22, 20, content::kKitCultist);
  y->level = 13;
  y->mp = 30;
  w.trySkill(*y, 11, y->id);
  CHECK(w.sanctCount() == 0);
  // broke: quiet fail
  server::Entity* b = spawnKit(w, "spent", 24, 20, content::kKitCultist);
  b->level = 14;
  b->mp = 19;
  w.trySkill(*b, 11, b->id);
  CHECK(w.sanctCount() == 0);
  CHECK(w.find(b->id)->mp == 19u);
  // inside the 600t CD: rejected (no second circle either)
  server::Entity* c = spawnKit(w, "chanter", 26, 20, content::kKitCultist);
  c->level = 14;
  c->mp = 60;
  w.trySkill(*c, 11, c->id);
  REQUIRE(w.sanctCount() == 1);
  w.trySkill(*w.find(c->id), 11, c->id);
  CHECK(w.sanctCount() == 1);
  CHECK(w.find(c->id)->mp == 60u - 20u);  // second cast spent nothing
}

TEST_CASE("T-161b.1: pulses knit the party inside radius 3") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "choir", 20, 20, content::kKitCultist);
  c->level = 14;  // pulse = (12+14)/2 = 13
  c->mp = 60;
  c->hp = 50;
  c->hpMax = 100;
  server::Entity* m = spawnKit(w, "mate", 22, 21, content::kKitCultist);
  REQUIRE(w.partyInvite(*c, *m));
  REQUIRE(w.partyAccept(*w.find(m->id)));
  m->hp = 40;
  m->hpMax = 100;
  server::Entity* far = spawnKit(w, "far", 20, 26, content::kKitCultist);
  far->hp = 10;  // d=6 > 3: outside the hold
  far->hpMax = 100;
  server::Entity* out = spawnKit(w, "stranger", 21, 20, content::kKitRavager);
  out->hp = 10;  // inside, but not of the party
  out->hpMax = 100;
  w.trySkill(*c, 11, c->id);
  for (int i = 0; i < 40; ++i) w.tick();
  // OOC regen drifts every bar; `far` is the regen control (outside hold).
  const std::uint32_t regen = w.find(far->id)->hp - 10u;
  CHECK(w.find(c->id)->hp == 50u + 13u + regen);
  CHECK(w.find(m->id)->hp == 40u + 13u + regen);
  CHECK(w.find(far->id)->hp == 10u + regen);  // regen only
  CHECK(w.find(out->id)->hp == 10u + regen);  // regen only, no hold
  CHECK(sawKind(w, 8));               // mend lane carries the pulses
}

TEST_CASE("T-161b.1: hold expires at 600t; recast moves, never stacks") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "holder", 20, 20, content::kKitCultist);
  c->level = 14;
  c->mp = 100;
  w.trySkill(*c, 11, c->id);
  REQUIRE(w.sanctCount() == 1);
  for (int i = 0; i < 600; ++i) w.tick();
  CHECK(w.sanctCount() == 0);  // the ground lets go
  // walk off and re-hallow: still exactly one circle
  w.find(c->id)->mp = 100;
  w.trySkill(*w.find(c->id), 11, c->id);
  CHECK(w.sanctCount() == 1);
}

TEST_CASE("T-161b.1: thin blood takes the pulse at 75% (Mend-mirror)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "choir", 20, 20, content::kKitCultist);
  c->level = 14;  // pulse 13 -> 9 under curse
  c->mp = 60;
  server::Entity* m = spawnKit(w, "mate", 21, 20, content::kKitCultist);
  REQUIRE(w.partyInvite(*c, *m));
  REQUIRE(w.partyAccept(*w.find(m->id)));
  m->hp = 40;
  m->hpMax = 100;
  m->curseUntil = w.tickCount() + 600;
  server::Entity* far = spawnKit(w, "control", 20, 30, content::kKitCultist);
  far->hp = 10;  // regen control, outside the hold
  far->hpMax = 100;
  w.trySkill(*c, 11, c->id);
  for (int i = 0; i < 40; ++i) w.tick();
  const std::uint32_t regen = w.find(far->id)->hp - 10u;
  CHECK(w.find(m->id)->hp == 40u + 13u * 75u / 100u + regen);
}

TEST_CASE("T-161b.1: chan 11 through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnKit(w, "relayed", 20, 20, content::kKitCultist);
  p->level = 14;
  p->mp = 30;
  server::Command cmd;
  cmd.kind = server::Command::kSkill;
  cmd.channel = 11;
  cmd.a = static_cast<std::int32_t>(p->id);
  server::applyWorldCommand(w, *p, cmd);
  CHECK(w.sanctCount() == 1);
}
