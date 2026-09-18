// T-161b.4 Corpse Explosion (chan 14): the Gravecaller's detonation.
// GDD §3 fixes only "detonates a corpse: AoE, costs the corpse"; every
// other number is flagged derivation (see world.cpp kBlast* block):
// unlock 14, MP 15, 120t CD, search 4, blast 2, fixed 20+3L, 300t meat.
// Mob corpses only (player corpses sacrosanct, thralls crumble). Fixed
// damage: zero RNG. Player kills run the full PK law (a red-choice AoE).
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

TEST_CASE("T-161b.4: blast spends the freshest meat for fixed AoE") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* g = spawnKit(w, "grave", 20, 20, content::kKitGravecaller);
  g->level = 14;  // blast = 20+3*14 = 62
  g->mp = 40;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  // two ghouls brawl: the survivor's victim tapes a corpse at (22,20).
  // T-106: capture ids — the refs dangle once anything dies.
  server::Entity& a = w.debugSpawnMob(*ghoul, sim::TilePos{22, 20});
  server::Entity& b = w.debugSpawnMob(*ghoul, sim::TilePos{23, 20});
  const std::uint32_t aId = a.id;
  a.hp = 1;
  b.attackTarget = aId;
  for (int i = 0; i < 300 && w.find(aId) != nullptr; ++i) w.tick();
  REQUIRE(w.find(aId) == nullptr);  // taped at (22,20)
  // a fresh ghoul shambles onto the corpse
  server::Entity& v = w.debugSpawnMob(*ghoul, sim::TilePos{22, 21});
  const std::uint32_t vId = v.id;
  v.hp = 60;
  v.hpMax = 64;
  const std::uint32_t xp0 = w.find(g->id)->xp;
  w.trySkill(*g, 14, g->id);
  CHECK(w.find(g->id)->mp == 40u - 15u);
  CHECK(w.find(vId) == nullptr);  // 62 >= 60: blasted apart
  CHECK(w.find(g->id)->xp > xp0);  // the caster's kill: full XP law
  CHECK(sawKind(w, 5));            // skill-hit lane carries the blast
}

TEST_CASE("T-161b.4: gates — kit, level, MP, CD, no meat") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  // Cultist has no rite on chan 14
  server::Entity* c = spawnKit(w, "choir", 20, 20, content::kKitCultist);
  c->level = 25;
  c->mp = 40;
  w.trySkill(*c, 14, c->id);
  CHECK(w.find(c->id)->mp == 40u);  // not even gated spend: wrong kit
  // below unlock 14: nothing
  server::Entity* y = spawnKit(w, "young", 22, 20, content::kKitGravecaller);
  y->level = 13;
  y->mp = 40;
  w.trySkill(*y, 14, y->id);
  CHECK(w.find(y->id)->mp == 40u);
  // broke: quiet fail
  server::Entity* b = spawnKit(w, "spent", 24, 20, content::kKitGravecaller);
  b->level = 14;
  b->mp = 14;
  w.trySkill(*b, 14, b->id);
  CHECK(w.find(b->id)->mp == 14u);
  // no meat within 4: quiet fail, nothing spent
  server::Entity* g = spawnKit(w, "grave", 26, 20, content::kKitGravecaller);
  g->level = 14;
  g->mp = 40;
  w.trySkill(*g, 14, g->id);
  CHECK(w.find(g->id)->mp == 40u);
}

TEST_CASE("T-161b.4: stale meat and kin immunity") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* g = spawnKit(w, "grave", 20, 20, content::kKitGravecaller);
  g->level = 14;
  g->mp = 100;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& a = w.debugSpawnMob(*ghoul, sim::TilePos{22, 20});
  server::Entity& b = w.debugSpawnMob(*ghoul, sim::TilePos{23, 20});
  const std::uint32_t aId = a.id;
  a.hp = 1;
  b.attackTarget = aId;
  for (int i = 0; i < 300 && w.find(aId) != nullptr; ++i) w.tick();
  REQUIRE(w.find(aId) == nullptr);
  for (int i = 0; i < 400; ++i) w.tick();  // meat goes stale (>300t)
  server::Entity& v = w.debugSpawnMob(*ghoul, sim::TilePos{22, 21});
  const std::uint32_t vId = v.id;
  v.hp = v.hpMax;
  w.trySkill(*w.find(g->id), 14, g->id);
  CHECK(w.find(g->id)->mp == 100u);  // stale: nothing spent
  CHECK(w.find(vId)->hp == v.hpMax);
}

TEST_CASE("T-161b.4: chan 14 through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* g = spawnKit(w, "relayed", 20, 20, content::kKitGravecaller);
  g->level = 14;
  g->mp = 40;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& a = w.debugSpawnMob(*ghoul, sim::TilePos{22, 20});
  server::Entity& b = w.debugSpawnMob(*ghoul, sim::TilePos{23, 20});
  const std::uint32_t aId = a.id;
  a.hp = 1;
  b.attackTarget = aId;
  for (int i = 0; i < 300 && w.find(aId) != nullptr; ++i) w.tick();
  REQUIRE(w.find(aId) == nullptr);
  server::Entity& v = w.debugSpawnMob(*ghoul, sim::TilePos{22, 21});
  const std::uint32_t vId = v.id;
  v.hp = 60;
  v.hpMax = 64;
  server::Command cmd;
  cmd.kind = server::Command::kSkill;
  cmd.channel = 14;
  cmd.a = static_cast<std::int32_t>(g->id);
  server::applyWorldCommand(w, *g, cmd);
  CHECK(w.find(vId) == nullptr);
}
