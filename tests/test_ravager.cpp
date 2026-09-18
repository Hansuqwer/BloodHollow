// T-161b.6 Ravager set (chan 19–23): Sunder (-15% DEF), Bull Rush (dash 3 +
// knockdown), War Stomp (PBAoE + stagger), Execute (aimed ×2 vs <20%),
// Second Wind (25% over 5 s, breaks on hit). GDD fixes the shapes; numbers
// are flagged derivation (see world.cpp kSunder*/kRush*/kStomp*/kExec*/kWind*
// block and kits.h unlocks 6/8/12/14/10). Execute is an AIMED channel (not
// a passive) so old journals — which never invoke ch22 — replay identical.
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

TEST_CASE("T-161b.6: sunder rends plate 15% (stacks with weakness)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* r = spawnKit(w, "rend", 20, 20, content::kKitRavager);
  r->level = 6;
  r->mp = 30;
  const content::MobDef* ghoul = content::findMob(1002);  // def 5
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{21, 20});
  const std::uint32_t mId = m.id;
  REQUIRE(w.effDef(m) == 5u);
  w.trySkill(*r, 19, mId);
  CHECK(w.find(mId)->sunderUntil > w.tickCount());
  CHECK(w.effDef(*w.find(mId)) == 5u * 85u / 100u);
  CHECK(sawKind(w, 25));
  // weakness stacks under it (0.85 x 0.85, flagged)
  w.find(mId)->weakUntil = w.tickCount() + 600;
  CHECK(w.effDef(*w.find(mId)) == 5u * 85u / 100u * 85u / 100u);
  // gates: below unlock 6, broke, out of reach (d=2 > 1), party kin
  server::Entity* y = spawnKit(w, "young", 24, 20, content::kKitRavager);
  y->level = 5;
  y->mp = 30;
  server::Entity& m2 = w.debugSpawnMob(*ghoul, sim::TilePos{24, 21});
  w.trySkill(*y, 19, m2.id);
  CHECK(w.find(m2.id)->sunderUntil == -1);
  server::Entity* mate = spawnKit(w, "mate", 20, 22, content::kKitRavager);
  REQUIRE(w.partyInvite(*r, *mate));
  REQUIRE(w.partyAccept(*w.find(mate->id)));
  w.find(r->id)->mp = 30;
  w.trySkill(*w.find(r->id), 19, mate->id);
  CHECK(w.find(mate->id)->sunderUntil == -1);
}

TEST_CASE("T-161b.6: bull rush dashes 3 and lays the mark prone") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* r = spawnKit(w, "bull", 20, 20, content::kKitRavager);
  r->level = 8;
  r->mp = 30;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{24, 20});
  const std::uint32_t mId = m.id;
  w.trySkill(*r, 20, mId);
  // dashed east along the path (3 tiles), mark prone 40t
  const sim::TilePos rp = w.find(r->id)->walker.tile();
  CHECK(rp.x > 20);
  CHECK(rp.x <= 23);
  CHECK(rp.y == 20);
  CHECK(w.find(mId)->knockUntil > w.tickCount());
  CHECK(sawKind(w, 24));
  // prone feet plant: no step for the next 10 ticks (movers only)
  w.find(mId)->attackTarget = 0;
  w.find(mId)->path.clear();
  const sim::TilePos mp0 = w.find(mId)->walker.tile();
  for (int i = 0; i < 10; ++i) w.tick();
  CHECK(w.find(mId)->walker.tile() == mp0);
  // prone blade refuses: a swung Power Swing lands nothing
  server::Entity* r2 = spawnKit(w, "second", 26, 20, content::kKitRavager);
  r2->level = 8;
  const std::uint32_t hp0 = w.find(mId)->hp;
  w.find(r2->id)->knockUntil = w.tickCount() + 100;
  w.trySkill(*w.find(r2->id), 1, mId);
  CHECK(w.find(mId)->hp == hp0);
}

TEST_CASE("T-161b.6: war stomp quakes radius 2, staggers, spares kin") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* r = spawnKit(w, "quake", 20, 20, content::kKitRavager);
  r->level = 12;
  r->mp = 40;
  const std::uint32_t base = w.effDmgBase(*r);
  const std::uint32_t quake = base * 75u / 100u;
  REQUIRE(quake > 0u);  // naked fists still shake the yard
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& a = w.debugSpawnMob(*ghoul, sim::TilePos{21, 20});
  server::Entity& b = w.debugSpawnMob(*ghoul, sim::TilePos{20, 22});
  server::Entity& far = w.debugSpawnMob(*ghoul, sim::TilePos{20, 25});
  const std::uint32_t aId = a.id, bId = b.id, fId = far.id;
  const std::uint32_t ha0 = a.hp, hb0 = b.hp, hf0 = far.hp;
  server::Entity* mate = spawnKit(w, "mate", 21, 21, content::kKitRavager);
  REQUIRE(w.partyInvite(*r, *mate));
  REQUIRE(w.partyAccept(*w.find(mate->id)));
  const std::uint32_t mateId = mate->id;
  const std::uint32_t hm0 = mate->hp;
  w.trySkill(*r, 21, r->id);
  CHECK(w.find(aId)->hp == ha0 - quake);
  CHECK(w.find(bId)->hp == hb0 - quake);
  CHECK(w.find(fId)->hp == hf0);  // d=5: untouched
  CHECK(w.find(mateId)->hp == hm0);  // kin immune
  CHECK(w.find(aId)->knockUntil > w.tickCount());  // staggered
  CHECK(w.find(r->id)->mp == 40u - 15u);
}

TEST_CASE("T-161b.6: execute doubles a same-state swing, only the bleeding") {
  // twin worlds, identical seeds: the only difference is ch1 vs ch22.
  // Same rolls => the finisher lands exactly twice the swing. A Revenant
  // (380 max) at 70 hp bleeds (18%) yet survives both blows for the math.
  const content::MobDef* rev = content::findMob(1018);
  REQUIRE(rev != nullptr);
  server::World wa;
  REQUIRE(wa.loadFrom(makeArena()));
  server::Entity* ra = spawnKit(wa, "plain", 20, 20, content::kKitRavager);
  ra->level = 14;
  server::Entity& ma = wa.debugSpawnMob(*rev, sim::TilePos{21, 20});
  const std::uint32_t maId = ma.id;
  ma.hp = 70;
  wa.trySkill(*ra, 1, maId);
  REQUIRE(wa.find(maId) != nullptr);  // plain swing never finishes here
  const std::uint32_t dmgA = 70u - wa.find(maId)->hp;

  server::World wb;
  REQUIRE(wb.loadFrom(makeArena()));
  server::Entity* rb = spawnKit(wb, "headsman", 20, 20, content::kKitRavager);
  rb->level = 14;
  rb->mp = 30;
  server::Entity& mb = wb.debugSpawnMob(*rev, sim::TilePos{21, 20});
  const std::uint32_t mbId = mb.id;
  mb.hp = 70;
  wb.trySkill(*rb, 22, mbId);
  REQUIRE(wb.find(mbId) != nullptr);  // doubled, still standing (70 hp mark)
  const std::uint32_t dmgB = 70u - wb.find(mbId)->hp;
  CHECK(dmgB == 2u * dmgA);
  CHECK(wb.find(rb->id)->mp == 30u - 8u);
  // healthy marks refuse the rite (nothing spent)
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& mh = wb.debugSpawnMob(*ghoul, sim::TilePos{19, 20});
  mh.hp = mh.hpMax;
  wb.find(rb->id)->mp = 30;
  for (int i = 0; i < 100; ++i) wb.tick();  // outwait nothing; CD is fresh
  wb.trySkill(*wb.find(rb->id), 22, mh.id);
  CHECK(wb.find(rb->id)->mp == 30u);
}

TEST_CASE("T-161b.6: second wind rallies 25%, breaks on blows") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* r = spawnKit(w, "rally", 20, 20, content::kKitRavager);
  r->level = 10;
  r->mp = 30;
  const std::uint32_t pool = static_cast<std::uint32_t>(r->hpMax) / 4u;
  REQUIRE(pool > 0u);
  w.find(r->id)->hp = w.find(r->id)->hpMax / 2u;
  const std::uint32_t hp0 = w.find(r->id)->hp;
  w.trySkill(*r, 23, r->id);
  CHECK(w.find(r->id)->mp == 30u - 10u);
  CHECK(sawKind(w, 8));
  for (int i = 0; i < 20; ++i) w.tick();  // one pulse
  const std::uint32_t pulse = static_cast<std::uint32_t>(r->hpMax) / 20u;
  CHECK(w.find(r->id)->hp == hp0 + pulse);
  // a blow after the cast-mark breaks the rally (mechanism pin: any hurt
  // tick past the mark ends it — strikes set lastHurtTick the same way)
  w.find(r->id)->lastHurtTick = w.tickCount();
  const std::uint32_t hp1 = w.find(r->id)->hp;
  for (int i = 0; i < 100; ++i) w.tick();
  CHECK(w.find(r->id)->hp == hp1);
}

TEST_CASE("T-161b.6: chan 19-23 through applyWorldCommand (replay path)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* r = spawnKit(w, "relayed", 20, 20, content::kKitRavager);
  r->level = 14;
  r->mp = 100;
  const content::MobDef* ghoul = content::findMob(1002);
  REQUIRE(ghoul != nullptr);
  server::Entity& m = w.debugSpawnMob(*ghoul, sim::TilePos{21, 20});
  const std::uint32_t mId = m.id;
  const std::uint32_t cmds[5][2] = {
      {19, mId}, {20, mId}, {21, 0}, {22, mId}, {23, 0}};
  for (const auto& c : cmds) {
    server::Command cmd;
    cmd.kind = server::Command::kSkill;
    cmd.channel = static_cast<std::uint8_t>(c[0]);
    cmd.a = static_cast<std::int32_t>(c[0] == 21 || c[0] == 23 ? r->id : c[1]);
    server::applyWorldCommand(w, *w.find(r->id), cmd);
  }
  CHECK(w.find(mId)->sunderUntil > w.tickCount());   // rend laid
  CHECK(w.find(mId)->knockUntil > w.tickCount());    // rushed or stomped prone
  CHECK(w.find(r->id)->windPool > 0u);               // rally held
}
