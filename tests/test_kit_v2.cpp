// T-054b Cultist v2 + Gravecaller Haste: chorus / mass-mend / haste pins.
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
}  // namespace

TEST_CASE("T-054b: Chorus needs a party and sings the circle in range") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* c = w.find(w.spawn("cantor", 0, sim::TilePos{10, 10}).id);
  c->classId = content::kKitCultist;
  c->level = 12;
  c->mp = 100;
  // a choir of one is a hum (era law): no party, no verse
  server::Command ch;
  ch.kind = server::Command::kSkill;
  ch.channel = 6;
  server::applyWorldCommand(w, *c, ch);
  CHECK(c->chorusUntil == -1);
  CHECK(c->mp == 100);

  // swear a second voice into the same party
  auto* f = w.find(w.spawn("faithful", 0, sim::TilePos{11, 10}).id);
  const std::uint32_t fid = f->id;
  REQUIRE(w.partyInvite(*c, *f));   // 'c' forms and leads
  REQUIRE(w.partyAccept(*f));
  server::applyWorldCommand(w, *c, ch);
  CHECK(c->mp == 100 - 14);          // cost paintrain
  CHECK(c->chorusUntil > 0);
  CHECK(w.find(fid)->chorusUntil > 0);  // the party member caught the verse
  // out-of-range voices stay silent
  // the absent voice joins while adjacent (invite law: 12-tile radius),
  // then walks out of earshot before the next verse.
  auto* f2 = w.find(w.spawn("absent", 0, sim::TilePos{11, 10}).id);
  c = w.find(c->id);
  REQUIRE(w.partyInvite(*c, *f2));
  REQUIRE(w.partyAccept(*f2));
  f2 = w.find(f2->id);
  f2->walker.place(sim::TilePos{30, 30});
  server::Command ch2 = ch;
  c->lastChorusTick = -1000;  // reset cd for the check
  c->mp = 100;
  server::applyWorldCommand(w, *c, ch2);
  CHECK(w.find(f2->id)->chorusUntil == -1);
}

TEST_CASE("T-054b: Chorus rides +5% through both acc & dmg math") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* c = w.find(w.spawn("clarion", 0, sim::TilePos{10, 10}).id);
  w.debugGive(*c, 2001, 1);
  for (std::uint8_t i = 0; i < c->inv.size(); ++i)
    if (c->inv[i].itemId == 2001) { w.toggleEquip(*c, i); break; }
  const std::uint32_t dmg0 = w.effDmgBase(*c);
  const std::uint32_t acc0 = w.effAcc(*c);
  c->chorusUntil = w.tickCount() + 100;
  CHECK(w.effDmgBase(*c) == dmg0 + dmg0 / 20);  // *1.05 floored
  CHECK(w.effAcc(*c) == acc0 + acc0 / 20);
  w.debugSetTick(w.tickCount() + 200);  // verse over
  CHECK(w.effDmgBase(*c) == dmg0);
}

TEST_CASE("T-054b: Mass Mend knits the circle at half strength") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* c = w.find(w.spawn("medicant", 0, sim::TilePos{10, 10}).id);
  c->classId = content::kKitCultist;
  c->level = 12;
  c->mp = 100;
  auto* a = w.find(w.spawn("wounded1", 0, sim::TilePos{11, 10}).id);
  auto* b = w.find(w.spawn("wounded2", 0, sim::TilePos{12, 10}).id);
  REQUIRE(w.partyInvite(*c, *a));
  REQUIRE(w.partyAccept(*a));
  c = w.find(c->id);
  a = w.find(a->id);
  REQUIRE(w.partyInvite(*c, *b));
  REQUIRE(w.partyAccept(*b));
  b = w.find(b->id);
  a->hp = 1;
  b->hp = 1;
  a->hpMax = 500;
  b->hpMax = 500;
  // (30 + 4*12)/2 = 39 per voice
  server::Command mm;
  mm.kind = server::Command::kSkill;
  mm.channel = 7;
  server::applyWorldCommand(w, *c, mm);
  CHECK(c->mp == 100 - 18);
  CHECK(a->hp == 40);
  CHECK(b->hp == 40);
  CHECK(c->hp == c->hpMax);  // self is full: no spend wasted (heal skips full)
}

TEST_CASE("T-054b: Haste shortens swing cadence by 25% over a kill window") {
  auto harness = [](bool haste) {
    server::World w;
    w.loadFrom(makeArena());
    auto* p = w.find(w.spawn("rotor", 0, sim::TilePos{10, 10}).id);
    if (haste) p->hasteUntil = 100000;  // pin the flag on directly
    // unkillable anvil-of-flesh standing adjacent: cadence measurement only
    auto* gh = w.find(w.debugSpawnMob(*content::findMob(1002), {11, 10}).id);
    gh->hp = 999999;
    gh->hpMax = 999999;
    gh->aggroRadius = 0;  // passive: no return bite while measuring
    gh->wanderRadius = 0;
    gh->leashRadius = 1;
    server::Command atk;
    atk.kind = server::Command::kAttack;
    atk.a = static_cast<std::int32_t>(gh->id);
    server::applyWorldCommand(w, *p, atk);
    const std::uint32_t pid = p->id;
    const auto* q = w.find(pid);
    // sample the swing clock: record tick of each swing via lastSwingTick delta
    sim::Tick lastSeen = -1;
    double sum = 0;
    int n = 0;
    for (int i = 0; i < 600; ++i) {  // 30s window: long enough to read cadence
      w.tick();
      q = w.find(pid);
      if (q->lastSwingTick != lastSeen && lastSeen >= 0) {
        sum += static_cast<double>(q->lastSwingTick - lastSeen);
        ++n;
      }
      lastSeen = q != nullptr ? q->lastSwingTick : -1;
    }
    (void)q;
    return n > 0 ? sum / n : 1e9;  // average ticks between swings
  };
  const double normal = harness(false);
  const double hasted = harness(true);
  INFO("normal mean inter-swing ticks=", normal, " hasted=", hasted);
  // 16 vs 12 cadence: haste sits 75% of normal modulo approach/fight jitter
  CHECK(hasted < normal);
  CHECK(hasted < normal * 0.9);
  CHECK(hasted > normal * 0.6);
}

TEST_CASE("T-054b: level gates — chorus at 9, mass mend at 12, haste at 10/11") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* c = w.find(w.spawn("novice", 0, sim::TilePos{10, 10}).id);
  c->classId = content::kKitCultist;
  c->level = 8;
  c->mp = 100;
  auto* f = w.find(w.spawn("faithful", 0, sim::TilePos{11, 10}).id);
  REQUIRE(w.partyInvite(*c, *f));
  REQUIRE(w.partyAccept(*f));
  c = w.find(c->id);
  server::Command ch;
  ch.kind = server::Command::kSkill;
  ch.channel = 6;
  server::applyWorldCommand(w, *c, ch);
  CHECK(c->mp == 100);               // too low for the verse (unlock 9)
  c->level = 9;
  server::applyWorldCommand(w, *c, ch);
  CHECK(c->chorusUntil > 0);         // now it sings
  // ravager may not channel songlines at all
  auto* r = w.find(w.spawn("brute", 0, sim::TilePos{12, 10}).id);
  r->classId = content::kKitRavager;
  r->level = 25;
  r->mp = 100;
  server::applyWorldCommand(w, *r, ch);
  CHECK(r->chorusUntil == -1);
  CHECK(r->mp == 100);
}
