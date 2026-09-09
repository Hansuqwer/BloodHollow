// T-082 Cultist Purify (chan 9): the field cleanse for thin blood.
// Pins mirror Mend exactly (CD 25t, 8 MP, 6 tiles, self-or-party): the only
// designed numbers are the channel (first free) and the Cultist unlock (6,
// utility-tier parity with Ironskin — flagged). Cleanses ONLY.
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
}  // namespace

TEST_CASE("T-082: purify clears the curse (self, full pin)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnKit(w, "cantor", 20, 20, content::kKitCultist);
  p->level = 6;
  p->mp = 30;
  p->hp = 50;
  p->hpMax = 94;
  p->curseUntil = w.tickCount() + 600;
  p->blessUntil = w.tickCount() + 6000;
  w.trySkill(*p, 9, p->id);
  CHECK(p->curseUntil == -1);
  CHECK(p->mp == 30u - 8u);  // Mend-rate mirror
  CHECK(p->hp == 50u);       // cleanse only: no heal
  CHECK(p->blessUntil > w.tickCount());  // ...and no bless touch
  // inside the 25t CD: rejected
  p->curseUntil = w.tickCount() + 600;
  w.trySkill(*w.find(p->id), 9, p->id);
  CHECK(w.find(p->id)->curseUntil > w.tickCount());
}

TEST_CASE("T-082: gates — kit, level, MP, range, clean blood") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  // Ravager has no rite on chan 9
  server::Entity* r = spawnKit(w, "sword", 20, 20, content::kKitRavager);
  r->mp = 30;
  r->curseUntil = w.tickCount() + 600;
  w.trySkill(*r, 9, r->id);
  CHECK(w.find(r->id)->curseUntil > w.tickCount());
  // Cultist below unlock 6: nothing
  server::Entity* y = spawnKit(w, "young", 22, 20, content::kKitCultist);
  y->level = 5;
  y->mp = 30;
  y->curseUntil = w.tickCount() + 600;
  w.trySkill(*y, 9, y->id);
  CHECK(w.find(y->id)->curseUntil > w.tickCount());
  // broke: quiet fail
  server::Entity* b = spawnKit(w, "spent", 24, 20, content::kKitCultist);
  b->level = 6;
  b->mp = 7;
  b->curseUntil = w.tickCount() + 600;
  w.trySkill(*b, 9, b->id);
  CHECK(w.find(b->id)->curseUntil > w.tickCount());
  // out of reach (Mend range 6): quiet fail
  server::Entity* c = spawnKit(w, "chanter", 20, 22, content::kKitCultist);
  c->level = 6;
  c->mp = 30;
  server::Entity* far = spawnKit(w, "far", 20, 30, content::kKitCultist);
  far->curseUntil = w.tickCount() + 600;
  w.trySkill(*c, 9, far->id);  // d=8 > 6, and not same party
  CHECK(w.find(far->id)->curseUntil > w.tickCount());
  // clean blood: nothing to purge (no MP spent)
  server::Entity* s = spawnKit(w, "clean", 26, 20, content::kKitCultist);
  s->level = 6;
  s->mp = 30;
  w.trySkill(*s, 9, s->id);
  CHECK(w.find(s->id)->mp == 30u);
}

TEST_CASE("T-082: party mate in range is cleansed") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "choir", 20, 20, content::kKitCultist);
  c->level = 6;
  c->mp = 30;
  server::Entity* m = spawnKit(w, "mate", 22, 20, content::kKitCultist);
  REQUIRE(w.partyInvite(*c, *m));
  REQUIRE(w.partyAccept(*w.find(m->id)));
  m->curseUntil = w.tickCount() + 600;
  w.trySkill(*c, 9, m->id);
  CHECK(w.find(m->id)->curseUntil == -1);
}

TEST_CASE("T-082: chan 9 through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnKit(w, "relayed", 20, 20, content::kKitCultist);
  p->level = 6;
  p->mp = 30;
  p->curseUntil = w.tickCount() + 600;
  server::Command cmd;
  cmd.kind = server::Command::kSkill;
  cmd.channel = 9;
  cmd.a = static_cast<std::int32_t>(p->id);
  server::applyWorldCommand(w, *p, cmd);
  CHECK(p->curseUntil == -1);
}
