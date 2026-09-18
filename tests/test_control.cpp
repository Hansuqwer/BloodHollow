// T-161b.5 Gravecaller control set (chan 15–18): Frost Spike (lesser nuke +
// 20% slow), Wither (DoT, stacks 3), Terror (2 s rout, bosses immune), Mana
// Shield (1 MP per 2 dmg). GDD fixes the shapes; numbers are flagged
// derivation (see world.cpp kFrost*/kWither*/kTerror*/kShield* block and
// kits.h unlocks 4/8/12/10). Zero RNG on old paths; new state unhashed
// (T-133 law).
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

TEST_CASE("T-161b.5: frost is the lesser nuke that leaves heavy feet") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* g = spawnKit(w, "frost", 20, 20, content::kKitGravecaller);
  g->level = 4;
  g->mp = 30;
  const std::uint32_t expected =
      5u + static_cast<std::uint32_t>(g->level) +
      static_cast<std::uint32_t>(g->intg);
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{22, 20});
  const std::uint32_t mId = m.id;
  const std::uint32_t hp0 = m.hp;
  w.trySkill(*g, 15, mId);
  CHECK(w.find(g->id)->mp == 30u - 6u);
  CHECK(w.find(mId)->hp == hp0 - expected);
  CHECK(w.find(mId)->slowUntil > w.tickCount());
  CHECK(sawKind(w, 5));
  // wrong kit: Cultist has no rite on chan 15
  server::Entity* c = spawnKit(w, "choir", 24, 20, content::kKitCultist);
  c->level = 25;
  c->mp = 30;
  server::Entity& m2 = w.debugSpawnMob(*ghoul, sim::TilePos{24, 22});
  const std::uint32_t hp2 = m2.hp;
  w.trySkill(*c, 15, m2.id);
  CHECK(w.find(m2.id)->hp == hp2);
}

TEST_CASE("T-161b.5: slowed feet move 4 ticks in 5") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnKit(w, "chilled", 10, 20, content::kKitRavager);
  server::Entity* b = spawnKit(w, "fleet", 10, 22, content::kKitRavager);
  // straight 10-tile marches east, walker-driven (no commands needed)
  for (int x = 11; x <= 20; ++x) {
    w.find(a->id)->path.push_back(sim::TilePos{x, 20});
    w.find(b->id)->path.push_back(sim::TilePos{x, 22});
  }
  w.find(a->id)->slowUntil = w.tickCount() + 1000;
  const int ax0 = w.find(a->id)->walker.x;
  const int bx0 = w.find(b->id)->walker.x;
  const std::uint32_t aid = w.find(a->id)->id;
  for (int i = 0; i < 20; ++i) w.tick();
  const int aWalked = w.find(a->id)->walker.x - ax0;
  const int bWalked = w.find(b->id)->walker.x - bx0;
  // tick 1 latches the path (no step); ticks 2-20 step unless chilled.
  // Skips land where (tick+id)%5==4 — computed from the live id.
  int skips = 0;
  for (int t = 2; t <= 20; ++t)
    if ((t + static_cast<int>(aid)) % 5 == 4) ++skips;
  CHECK(bWalked == 19 * 256);  // full pace: 256 units/tick
  CHECK(aWalked == (19 - skips) * 256);  // chilled pace
  CHECK(skips > 0);  // the chill actually bit (else the pin is vacuous)
}

TEST_CASE("T-161b.5: wither stacks to 3, pulses, expires, pays the caster") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* g = spawnKit(w, "rot", 20, 20, content::kKitGravecaller);
  g->level = 8;  // pulse = (2+8/4) = 4 per stack per 20t
  g->mp = 100;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{22, 20});
  const std::uint32_t mId = m.id;
  const std::uint32_t hp0 = m.hp;
  w.trySkill(*g, 16, mId);
  CHECK(w.find(mId)->witherStacks == 1u);
  CHECK(sawKind(w, 23));
  for (int i = 0; i < 20; ++i) w.tick();
  CHECK(w.find(mId)->hp == hp0 - 4u);
  // re-cast stacks (CD outwaited) and refreshes; the pulses between casts
  // keep coming (20t cadence): -4 at t20, -12 across t40-80, -8 at t100.
  for (int i = 0; i < 60; ++i) w.tick();
  w.find(g->id)->mp = 100;
  w.trySkill(*w.find(g->id), 16, mId);
  CHECK(w.find(mId)->witherStacks == 2u);
  for (int i = 0; i < 20; ++i) w.tick();
  CHECK(w.find(mId)->hp == hp0 - 4u - 12u - 8u);
  // expiry clears the stacks
  for (int i = 0; i < 240; ++i) w.tick();
  server::Entity* fm = w.find(mId);
  if (fm != nullptr) CHECK(fm->witherStacks == 0u);
}

TEST_CASE("T-161b.5: wither kill credits the caster") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* g = spawnKit(w, "rot", 20, 20, content::kKitGravecaller);
  g->level = 8;
  g->mp = 100;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{22, 20});
  const std::uint32_t mId = m.id;
  m.hp = 3;  // one pulse decides it
  const std::uint32_t xp0 = w.find(g->id)->xp;
  w.trySkill(*g, 16, mId);
  for (int i = 0; i < 40 && w.find(mId) != nullptr; ++i) w.tick();
  CHECK(w.find(mId) == nullptr);
  CHECK(w.find(g->id)->xp > xp0);
}

TEST_CASE("T-161b.5: terror routs, breaks aim, spares bosses") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* g = spawnKit(w, "fear", 20, 20, content::kKitGravecaller);
  g->level = 12;
  g->mp = 40;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{22, 20});
  const std::uint32_t mId = m.id;
  m.attackTarget = g->id;  // mid-swing when horror lands
  w.trySkill(*g, 17, mId);
  CHECK(w.find(mId)->fearUntil > w.tickCount());
  CHECK(w.find(mId)->attackTarget == 0u);
  CHECK(sawKind(w, 21));
  const sim::TilePos p0 = w.find(mId)->walker.tile();
  for (int i = 0; i < 40; ++i) w.tick();
  // routed east, away from the caster at (20,20)
  CHECK(w.find(mId)->walker.tile().x >= p0.x);
  // the Mother does not run from anyone
  const content::MobDef* mother = content::findMob(1009);
  REQUIRE(mother != nullptr);
  REQUIRE(mother->boss == 1);
  server::Entity& boss = w.debugSpawnMob(*mother, sim::TilePos{24, 20});
  w.find(g->id)->mp = 40;
  w.trySkill(*w.find(g->id), 17, boss.id);
  CHECK(w.find(boss.id)->fearUntil == -1);
}

TEST_CASE("T-161b.5: ward drinks MP first, never below zero purse") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* g = spawnKit(w, "grave", 20, 20, content::kKitGravecaller);
  g->level = 12;
  g->mp = 40;
  w.trySkill(*g, 18, g->id);
  CHECK(w.find(g->id)->shieldUntil > w.tickCount());
  CHECK(w.find(g->id)->mp == 40u - 10u);
  CHECK(sawKind(w, 22));
  // incoming Firebolt 8+2*4+2*INT from a lesser grave, PvP-scaled (victim
  // is a player), fully drunk by the 30 MP ward
  server::Entity* f = spawnKit(w, "foe", 24, 20, content::kKitGravecaller);
  f->level = 4;
  f->mp = 30;
  const std::uint32_t bolt =
      (8u + 2u * static_cast<std::uint32_t>(f->level) +
       2u * static_cast<std::uint32_t>(f->intg)) *
      65u / 100u;
  const std::uint32_t hp0 = w.find(g->id)->hp;
  const std::uint32_t mp0 = w.find(g->id)->mp;  // 30
  w.trySkill(*f, 5, g->id);
  const std::uint32_t drunk = bolt < mp0 * 2u ? bolt : mp0 * 2u;
  CHECK(w.find(g->id)->hp == hp0 - (bolt - drunk));
  CHECK(w.find(g->id)->mp == mp0 - (drunk + 1u) / 2u);
}

TEST_CASE("T-161b.5: ward lapses at 1200t") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* g = spawnKit(w, "grave", 20, 20, content::kKitGravecaller);
  g->level = 12;
  g->mp = 40;
  w.trySkill(*g, 18, g->id);
  REQUIRE(w.find(g->id)->shieldUntil > w.tickCount());
  for (int i = 0; i < 1200; ++i) w.tick();
  server::Entity* f = spawnKit(w, "foe", 24, 20, content::kKitGravecaller);
  f->level = 4;
  f->mp = 30;
  const std::uint32_t bolt =
      (8u + 2u * static_cast<std::uint32_t>(f->level) +
       2u * static_cast<std::uint32_t>(f->intg)) *
      65u / 100u;  // PvP-scaled: the victim is a player
  const std::uint32_t hp0 = w.find(g->id)->hp;
  const std::uint32_t mp0 = w.find(g->id)->mp;
  w.trySkill(*f, 5, g->id);
  CHECK(w.find(g->id)->hp == hp0 - bolt);
  CHECK(w.find(g->id)->mp == mp0);  // lapsed ward drinks nothing
}

TEST_CASE("T-161b.5: chan 15-18 through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* g = spawnKit(w, "relayed", 20, 20, content::kKitGravecaller);
  g->level = 12;
  g->mp = 100;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{22, 20});
  const std::uint32_t mId = m.id;
  for (std::uint8_t ch = 15; ch <= 18; ++ch) {
    server::Command cmd;
    cmd.kind = server::Command::kSkill;
    cmd.channel = ch;
    cmd.a = ch == 18 ? static_cast<std::int32_t>(g->id)
                     : static_cast<std::int32_t>(mId);
    server::applyWorldCommand(w, *w.find(g->id), cmd);
  }
  CHECK(w.find(mId)->slowUntil > w.tickCount());      // frost landed
  CHECK(w.find(mId)->witherStacks == 1u);             // wither laid
  CHECK(w.find(mId)->fearUntil > w.tickCount());      // terror took
  CHECK(w.find(g->id)->shieldUntil > w.tickCount());  // ward raised
}
