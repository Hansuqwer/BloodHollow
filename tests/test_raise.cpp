// T-161b.3 Raise Skeleton (chan 13): the Cultist's thrall.
// GDD §3 fixes only "1 pet"; every other number is flagged derivation
// (see world.cpp kRaise* block): unlock 16, MP 25, 600t CD, hp 30+10L,
// strikes 2+L, guard L/4, heel 2 / warp 18. kMob + ownerId: zero wire,
// zero hash (generic entity mix covers the body). Zero new RNG draws on
// old paths — thrall strikes roll only where no journal ever went.
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

TEST_CASE("T-161b.3: raise wakes a scaled thrall (bond, hp, signal)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "caller", 20, 20, content::kKitCultist);
  c->level = 16;
  c->mp = 40;
  const std::size_t n0 = w.count();
  w.trySkill(*c, 13, c->id);
  CHECK(w.count() == n0 + 1);
  const server::Entity* pet = w.petOf(c->id);
  REQUIRE(pet != nullptr);
  CHECK(pet->kind == server::EntityKind::kMob);
  CHECK(pet->mobId == content::kThrallMobId);
  CHECK(pet->hpMax == 30u + 10u * 16u);
  CHECK(pet->hp == pet->hpMax);
  CHECK(pet->petLevel == 16);
  CHECK(pet->zoneId == c->zoneId);
  CHECK(w.find(c->id)->mp == 40u - 25u);
  CHECK(sawKind(w, 19));
}

TEST_CASE("T-161b.3: gates — kit, level, MP, CD") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  // Ravager has no rite on chan 13
  server::Entity* r = spawnKit(w, "sword", 20, 20, content::kKitRavager);
  r->level = 25;
  r->mp = 40;
  w.trySkill(*r, 13, r->id);
  CHECK(w.petOf(r->id) == nullptr);
  // below unlock 16: nothing
  server::Entity* y = spawnKit(w, "young", 22, 20, content::kKitCultist);
  y->level = 15;
  y->mp = 40;
  w.trySkill(*y, 13, y->id);
  CHECK(w.petOf(y->id) == nullptr);
  // broke: quiet fail
  server::Entity* b = spawnKit(w, "spent", 24, 20, content::kKitCultist);
  b->level = 16;
  b->mp = 24;
  w.trySkill(*b, 13, b->id);
  CHECK(w.petOf(b->id) == nullptr);
  CHECK(w.find(b->id)->mp == 24u);
  // inside the 600t CD: refused
  server::Entity* c = spawnKit(w, "caller", 26, 20, content::kKitCultist);
  c->level = 16;
  c->mp = 100;
  w.trySkill(*c, 13, c->id);
  REQUIRE(w.petOf(c->id) != nullptr);
  const std::uint32_t pet0 = w.petOf(c->id)->id;
  w.trySkill(*w.find(c->id), 13, c->id);
  CHECK(w.petOf(c->id)->id == pet0);  // same thrall, no cycle
  CHECK(w.find(c->id)->mp == 100u - 25u);
}

TEST_CASE("T-161b.3: one-pet law — recast past CD replaces, never stacks") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "caller", 20, 20, content::kKitCultist);
  c->level = 16;
  c->mp = 200;
  w.trySkill(*c, 13, c->id);
  const std::uint32_t pet0 = w.petOf(c->id)->id;
  const std::size_t n0 = w.count();
  for (int i = 0; i < 600; ++i) w.tick();  // outwait the CD
  w.find(c->id)->mp = 200;
  w.trySkill(*w.find(c->id), 13, c->id);
  CHECK(w.count() == n0);  // old crumbled, one rose
  REQUIRE(w.petOf(c->id) != nullptr);
  CHECK(w.petOf(c->id)->id != pet0);
}

TEST_CASE("T-161b.3: thrall takes the master's mark, kill credits the bond") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "caller", 20, 20, content::kKitCultist);
  c->level = 16;
  c->mp = 40;
  c->hp = c->hpMax;
  w.trySkill(*c, 13, c->id);
  REQUIRE(w.petOf(c->id) != nullptr);
  const std::uint32_t petId = w.petOf(c->id)->id;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{21, 20});
  const std::uint32_t mId = m.id;
  m.hp = 1;  // one clean strike decides it
  const std::uint32_t xp0 = w.find(c->id)->xp;
  const std::uint8_t lvl0 = w.find(c->id)->level;
  w.find(c->id)->attackTarget = mId;  // the master marks it...
  bool dead = false;
  for (int i = 0; i < 200 && !dead; ++i) {
    w.tick();
    dead = w.find(mId) == nullptr;
  }
  REQUIRE(dead);  // ...and the thrall does the killing
  // the bond got paid (XP moved to the master, not the void)
  const bool paid = w.find(c->id)->xp > xp0 || w.find(c->id)->level > lvl0;
  CHECK(paid);
  // ...while the thrall itself is unharmed by the bookkeeping
  CHECK(w.find(petId) != nullptr);
  bool thrallLine = false;
  for (const auto& ev : w.events())
    if (ev.kind == 3 && ev.chatText.find("'s thrall has slain a ") != std::string::npos)
      thrallLine = true;
  CHECK(thrallLine);
}

TEST_CASE("T-161b.3: thrall death pays nothing (anti-funnel law)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "caller", 20, 20, content::kKitCultist);
  c->level = 16;
  c->mp = 40;
  w.trySkill(*c, 13, c->id);
  REQUIRE(w.petOf(c->id) != nullptr);
  const std::uint32_t petId = w.petOf(c->id)->id;
  const std::uint32_t gold0 = w.find(c->id)->gold;
  const std::size_t inv0 = w.find(c->id)->inv.size();
  // bleed the thrall to 1 hp, then let a ghoul finish it
  w.find(petId)->hp = 1;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& g = w.debugSpawnMob(*ghoul, sim::TilePos{21, 20});
  g.attackTarget = petId;
  bool gone = false;
  for (int i = 0; i < 600 && !gone; ++i) {
    w.tick();
    gone = w.find(petId) == nullptr;
  }
  REQUIRE(gone);
  CHECK(w.find(c->id)->gold == gold0);  // no purse from the pyre
  CHECK(w.find(c->id)->inv.size() == inv0);
  CHECK(w.petOf(c->id) == nullptr);
}

TEST_CASE("T-161b.3: leaving takes the thrall (logout path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "caller", 20, 20, content::kKitCultist);
  c->level = 16;
  c->mp = 40;
  w.trySkill(*c, 13, c->id);
  REQUIRE(w.petOf(c->id) != nullptr);
  w.despawn(c->id);
  CHECK(w.petOf(c->id) == nullptr);
}

TEST_CASE("T-161b.3: chan 13 through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* c = spawnKit(w, "relayed", 20, 20, content::kKitCultist);
  c->level = 16;
  c->mp = 40;
  server::Command cmd;
  cmd.kind = server::Command::kSkill;
  cmd.channel = 13;
  cmd.a = static_cast<std::int32_t>(c->id);
  server::applyWorldCommand(w, *c, cmd);
  CHECK(w.petOf(c->id) != nullptr);
}
