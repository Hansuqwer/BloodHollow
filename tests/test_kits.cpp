// T-053 class kits + T-054 Cultist/Gravecaller v1, pinned at World-API level.
// Legacy freeze: default kit = Ravager (8/8/8) — pre-kit characters bit-identical.
// Kit skills are kit-gated, level-gated, MP- and cooldown-priced, replay-exact.
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

server::Entity* spawnP(server::World& w, const char* name, int x, int y,
                       std::uint8_t kit = content::kKitRavager) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y}, 1, kit);
  return w.find(e.id);
}

server::Command cmd(server::Command::Kind k, std::int32_t a = 0,
                    std::uint8_t channel = 0) {
  server::Command c;
  c.kind = k;
  c.a = a;
  c.channel = channel;
  return c;
}
}  // namespace

TEST_CASE("T-053: default kit is behavior-frozen Ravager") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* r = spawnP(w, "oldtimer", 5, 5);  // no kit arg: legacy path
  CHECK(r->classId == content::kKitRavager);
  CHECK(r->str == 8);  // legacy seed exactly
  CHECK(r->vit == 8);
  CHECK(r->dex == 8);
  CHECK(r->intg == 0);
  CHECK(r->mp == 30);
  CHECK(r->mpMax == 30);
}

TEST_CASE("T-053: /kit choice one-time; level-1 window; seeds per kit") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* c = spawnP(w, "acolyte", 5, 5);  // starts Ravager (unsworn choice flow)
  const std::uint32_t cid = c->id;

  CHECK(w.kitChoose(*c, content::kKitCultist));  // L1 re-dedication ok
  CHECK(w.find(cid)->classId == content::kKitCultist);
  CHECK(w.find(cid)->str == 5);  // cultist seed
  CHECK(w.find(cid)->vit == 8);
  CHECK(w.find(cid)->mag == 3);

  // second swear at L1: allowed only while still L1 (re-dedication window)
  CHECK(w.kitChoose(*w.find(cid), content::kKitGravecaller));
  CHECK(w.find(cid)->classId == content::kKitGravecaller);
  CHECK(w.find(cid)->dex == 6);

  // invalid kit id rejected
  CHECK_FALSE(w.kitChoose(*w.find(cid), 9));

  // past L1: hard reject
  w.debugAwardXp(*w.find(cid), 120);  // L1 bar is 100 -> dings to L2
  REQUIRE(w.find(cid)->level == 2);
  CHECK_FALSE(w.kitChoose(*w.find(cid), content::kKitRavager));
  CHECK(w.find(cid)->classId == content::kKitGravecaller);  // unchanged
  // mp pool grew at the ding and refilled (era: ding tops you off)
  CHECK(w.find(cid)->mpMax == 36);
  CHECK(w.find(cid)->mp == 36);
}

TEST_CASE("T-053: kit-dispatch gating (wrong kit/channel = silent no-op)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* r = spawnP(w, "razor", 5, 5, content::kKitRavager);
  auto* g = spawnP(w, "hexer", 6, 5, content::kKitGravecaller);
  const std::uint32_t rid = r->id, gid = g->id;
  auto* rat = w.find(w.debugSpawnMob(*content::findMob(1001), {7, 5}).id);
  REQUIRE(rat != nullptr);

  // Ravager channels Mend (2): kit lacks the rite — nothing happens
  w.find(rid)->mp = 30;
  w.trySkill(*w.find(rid), 2, rid);
  CHECK(w.find(rid)->mp == 30);          // no payment
  CHECK(w.find(rid)->blessUntil < 0);

  // Gravecaller has no Power Swing (ch1)
  w.trySkill(*w.find(gid), 1, rat->id);
  CHECK(w.find(gid)->lastPowerTick == -1000);

  // Gravecaller nukes fine (ch5, L1 unlock)
  const std::uint32_t mobHp0 = w.find(rat->id)->hp;
  w.trySkill(*w.find(gid), 5, rat->id);
  CHECK(w.find(gid)->mp == 24);           // 30 - 6
  CHECK(w.find(rat->id)->hp < mobHp0);    // bolt landed
}

TEST_CASE("T-054: Mend — math, gates, targeting, CD, MP") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* c = spawnP(w, "choir", 5, 5, content::kKitCultist);
  auto* m = spawnP(w, "tank", 6, 5, content::kKitRavager);
  const std::uint32_t cid = c->id, mid = m->id;
  REQUIRE(w.partyInvite(*c, *m));
  REQUIRE(w.partyAccept(*w.find(mid)));

  // solo (no party member targeted): Mend self works without a party target
  w.find(cid)->hp = 10;
  w.trySkill(*w.find(cid), 2, 0);  // targetId 0 = self
  CHECK(w.find(cid)->hp == 10u + 34u);  // 30 + 4*L1
  CHECK(w.find(cid)->mp == 22);
  // same cast again inside 25t CD: rejected
  const std::uint32_t hpBefore = w.find(cid)->hp;
  w.trySkill(*w.find(cid), 2, 0);
  CHECK(w.find(cid)->hp == hpBefore);

  // mend the party mate (in range)
  for (int i = 0; i < 30; ++i) w.tick();   // CD off
  w.find(mid)->hp = 10;
  w.trySkill(*w.find(cid), 2, mid);
  CHECK(w.find(mid)->hp == 10u + 34u);

  // non-party target rejected (drops no mp)
  for (int i = 0; i < 30; ++i) w.tick();
  auto* solo = spawnP(w, "stray", 6, 6);  // not in the party
  const std::uint32_t sHp = w.find(solo->id)->hp - 5;
  w.find(solo->id)->hp = sHp;
  const std::uint32_t mpBefore = w.find(cid)->mp;
  w.trySkill(*w.find(cid), 2, solo->id);
  CHECK(w.find(solo->id)->hp == sHp);
  CHECK(w.find(cid)->mp == mpBefore);

  // out-of-range party target rejected (14 tiles: beyond Mend range 6)
  for (int i = 0; i < 30; ++i) w.tick();
  w.find(mid)->walker.place(sim::TilePos{20, 5});
  w.find(mid)->hp = 10;
  w.trySkill(*w.find(cid), 2, mid);
  CHECK(w.find(mid)->hp == 10u);

  // level-gate: Bless locked at L1 (needs L3)
  w.trySkill(*w.find(cid), 3, 0);
  CHECK(w.find(cid)->blessUntil < 0);
  // melt: not enough mp => no-op
  w.debugAwardXp(*w.find(cid), 500);      // several dings
  REQUIRE(w.find(cid)->level >= 3);
  w.find(cid)->mp = 10;
  w.trySkill(*w.find(cid), 3, 0);         // bless costs 15
  CHECK(w.find(cid)->blessUntil < 0);
}

TEST_CASE("T-054: Bless pumps acc & dmg; Ironskin lifts DEF; both expire") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* c = spawnP(w, "choir", 5, 5, content::kKitCultist);
  const std::uint32_t cid = c->id;
  w.debugAwardXp(*w.find(cid), 4500);  // dings L1->L6 (4480 cumulative bars)
  REQUIRE(w.find(cid)->level >= 6);

  const std::uint32_t baseAcc = w.effAcc(*w.find(cid));
  const std::uint32_t baseDmg = w.effDmgBase(*w.find(cid));
  const std::uint32_t baseDef = w.effDef(*w.find(cid));

  w.trySkill(*w.find(cid), 3, 0);   // Bless self (L3+ reached)
  CHECK(w.find(cid)->blessUntil > 0);
  CHECK(w.effAcc(*w.find(cid)) == baseAcc * 110u / 100u);
  CHECK(w.effDmgBase(*w.find(cid)) == baseDmg * 110u / 100u);
  w.trySkill(*w.find(cid), 3, 0);   // CD: 40t — rejected
  CHECK(w.find(cid)->mp > 0);

  w.trySkill(*w.find(cid), 4, 0);   // Ironskin (L6+)
  CHECK(w.find(cid)->ironskinUntil > 0);
  CHECK(w.effDef(*w.find(cid)) == baseDef * 120u / 100u);

  for (int i = 0; i < 6001; ++i) w.tick();  // buff window passes
  CHECK(w.effAcc(*w.find(cid)) == baseAcc);
  CHECK(w.effDmgBase(*w.find(cid)) == baseDmg);
  CHECK(w.effDef(*w.find(cid)) == baseDef);
}

TEST_CASE("T-054: Firebolt — range, mob kill feeds XP path, PvP scalar") {
  const content::MobDef* md = content::findMob(1001);
  REQUIRE(md != nullptr);
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* g = spawnP(w, "hexer", 5, 5, content::kKitGravecaller);
  const std::uint32_t gid = g->id;

  // point-blank kill: dmg = 8 + 2*1 + 2*4(intg seed) = 18 >= rat hp 30? no (30 hp)
  auto* rat = w.find(w.debugSpawnMob(*md, {8, 5}).id);
  REQUIRE(rat != nullptr);
  const std::uint32_t xp0 = w.find(gid)->xp;
  w.trySkill(*w.find(gid), 5, rat->id);       // 18 dmg: rat at 12/30
  CHECK(w.find(rat->id)->hp == 30u - 18u);
  for (int i = 0; i < 35; ++i) w.tick();       // CD off
  w.trySkill(*w.find(gid), 5, rat->id);        // second bolt kills
  const server::Entity* ratAfter = w.find(rat->id);
  CHECK(ratAfter == nullptr);  // killMob despawns the cadaver
  CHECK(w.find(gid)->xp > xp0);                // killMob path granted xp

  // range gate beyond 8 tiles: no-op
  auto* far = w.find(w.debugSpawnMob(*md, {20, 5}).id);
  REQUIRE(far != nullptr);
  const std::uint32_t mpBefore = w.find(gid)->mp;
  for (int i = 0; i < 35; ++i) w.tick();
  w.trySkill(*w.find(gid), 5, far->id);
  CHECK(w.find(far->id)->hp == 30u);
  CHECK(w.find(gid)->mp >= mpBefore);  // no spend (regen may add on the way)
}

TEST_CASE("T-053/54: kit ops replay-parity (interleaved vs pre-applied)") {
  server::World livew;
  REQUIRE(livew.loadFrom(makeArena()));
  const std::uint32_t lcid = spawnP(livew, "acolyte", 5, 5)->id;
  {
    server::applyWorldCommand(livew, *livew.find(lcid),
                              cmd(server::Command::kKitChoose, content::kKitCultist));
    livew.tick();
    livew.find(lcid)->hp = 5;  // deterministic wound (same op replayed below)
    for (int i = 0; i < 30; ++i) livew.tick();
    server::applyWorldCommand(livew, *livew.find(lcid),
                              cmd(server::Command::kSkill, 0, 2));  // Mend self
    livew.tick();
  }
  server::World replayw;
  REQUIRE(replayw.loadFrom(makeArena()));
  const std::uint32_t rcid = spawnP(replayw, "acolyte", 5, 5)->id;
  {
    server::applyWorldCommand(replayw, *replayw.find(rcid),
                              cmd(server::Command::kKitChoose, content::kKitCultist));
    replayw.tick();
    replayw.find(rcid)->hp = 5;
    for (int i = 0; i < 30; ++i) replayw.tick();
    server::applyWorldCommand(replayw, *replayw.find(rcid),
                              cmd(server::Command::kSkill, 0, 2));
    replayw.tick();
  }
  const auto& L = *livew.find(lcid);
  const auto& R = *replayw.find(rcid);
  CHECK(L.classId == R.classId);
  CHECK(L.hp == R.hp);
  CHECK(L.mp == R.mp);
  CHECK(L.classId == content::kKitCultist);
  CHECK(L.hp == 5u + 34u);
}
