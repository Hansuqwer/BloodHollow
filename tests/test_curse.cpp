// T-070 Blood Curse + chapel cure: the bolt leaves thin blood.
// Era pins: a boss Blood Bolt hit curses for 30 s (600 ticks); potions and
// Mend land at 75% while cursed; OOC regen untouched; the confessor clears
// on /confess within 3 tiles. Bless is NOT scaled — it heals nothing in
// this tree (a +10% hit&dmg buff), so there is no heal to thin. The curse
// lane runs through applyWorldCommand (kConfess), so live and replay share
// the branch. Premise correction: only the Gravemother (1009) carries the
// T-064 bolt kit in this tree — the Gravecaller (1007) has boss=0/boltRange=0
// and never casts. The source is the shared boss-bolt path, not a per-mob
// special.
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
#include "content/items.h"
#include "content/kits.h"
#include "content/mobs.h"
#include "sim/combat.h"
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
}  // namespace

TEST_CASE("T-070: the mother's bolt leaves thin blood for 600 ticks") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const content::MobDef* mother = content::findMob(1009);
  REQUIRE(mother != nullptr);
  REQUIRE(mother->boss == 1);
  server::Entity& m = w.debugSpawnMob(*mother, sim::TilePos{10, 10});
  server::Entity* p = spawnP(w, "cursed", 13, 10);  // d=3: bolt lane, not melee
  const std::uint32_t pid = p->id;
  bool cursed = false;
  for (int i = 0; i < 200 && !cursed; ++i) {
    w.tick();
    cursed = w.find(pid)->curseUntil > w.tickCount();
  }
  REQUIRE(cursed);
  // the stamp lands exactly one curse-window past the casting tick
  CHECK(w.find(pid)->curseUntil - w.tickCount() <= 600);
  CHECK(w.find(pid)->curseUntil - w.tickCount() > 590);
  (void)m;
}

TEST_CASE("T-070: potions run thin at 75% while cursed") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "sipper", 20, 20);
  p->hpMax = 94;
  p->hp = 54;  // room 40 = exactly one Blood Vial
  REQUIRE(w.debugGive(*p, 3001, 1));
  p->curseUntil = w.tickCount() + 600;
  REQUIRE(w.useItem(*p, 0));
  CHECK(p->hp == 54u + 30u);  // 40 * 75% = 30

  // clean control: the same vial heals whole
  server::World w2;
  REQUIRE(w2.loadFrom(makeArena()));
  server::Entity* q = spawnP(w2, "clean", 20, 20);
  q->hpMax = 94;
  q->hp = 54;
  REQUIRE(w2.debugGive(*q, 3001, 1));
  REQUIRE(w2.useItem(*q, 0));
  CHECK(q->hp == 94u);
}

TEST_CASE("T-070: Mend runs thin at 75% while cursed") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& e = w.spawn("mended", 0, sim::TilePos{20, 20}, 1,
                              content::kKitCultist);
  server::Entity* p = w.find(e.id);
  p->hpMax = 94;
  p->hp = 60;  // room 34 = exactly one L1 Mend (30 + 4*1)
  p->mp = 30;
  p->curseUntil = w.tickCount() + 600;
  w.trySkill(*p, 2, p->id);  // Mend self
  CHECK(p->hp == 60u + 25u);  // 34 * 75% = 25 (era-plain integer math)
}

TEST_CASE("T-070: exact expiry at the 600-tick edge") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "edgecase", 20, 20);
  p->hpMax = 94;
  p->hp = 54;
  REQUIRE(w.debugGive(*p, 3001, 2));
  p->curseUntil = 100;
  w.debugSetTick(99);
  REQUIRE(w.useItem(*p, 0));
  CHECK(p->hp == 54u + 30u);  // still cursed one tick before the edge
  w.debugSetTick(110);  // past the edge (and past the 10t sip CD)
  p->hp = 54;
  REQUIRE(w.useItem(*p, 0));
  CHECK(p->hp == 94u);  // past edge: whole blood again
}

TEST_CASE("T-070: the chapel takes the thin blood away (/confess)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  (void)w.debugSpawnConfessor(sim::TilePos{10, 10});
  server::Entity* p = spawnP(w, "penitent", 11, 10);
  p->karma = -5;
  p->curseUntil = w.tickCount() + 600;
  const std::int32_t karmaBefore = p->karma;
  REQUIRE(w.nearConfessor(*p));
  CHECK(w.confess(*p));
  CHECK(p->curseUntil == -1);
  CHECK(p->karma == karmaBefore);  // repentance is a later card: karma unmoved
  CHECK_FALSE(w.confess(*p));      // nothing left to shrive

  // too far from the chapel: the words find no purchase
  server::Entity* q = spawnP(w, "far", 30, 30);
  q->curseUntil = w.tickCount() + 600;
  CHECK_FALSE(w.confess(*q));
  CHECK(q->curseUntil > w.tickCount());
}

TEST_CASE("T-070: /confess through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  (void)w.debugSpawnConfessor(sim::TilePos{10, 10});
  server::Entity* p = spawnP(w, "roundtrip", 10, 11);
  p->curseUntil = w.tickCount() + 600;
  server::Command c;
  c.kind = server::Command::kConfess;
  server::applyWorldCommand(w, *p, c);
  CHECK(p->curseUntil == -1);
}

TEST_CASE("T-070: the town seeds exactly one Confessor in the chapel rect") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  std::size_t confessors = 0;
  const server::Entity* conf = nullptr;
  for (const auto& e : w.entities())
    if (e.wireKind == content::kWireKindConfessor) {
      ++confessors;
      conf = &e;
    }
  REQUIRE(confessors == 1u);
  CHECK(conf->name == "Confessor");
  const sim::TilePos t = conf->walker.tile();
  CHECK(t.x >= 9);
  CHECK(t.x <= 14);
  CHECK(t.y >= 9);
  CHECK(t.y <= 13);
}
