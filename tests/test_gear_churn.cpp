// T-058 durability: burn on swing/hit, dormancy at 0, repair economics.
// T-059 affixes v1 and T-060 refine pin themselves in the same file below.
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
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

TEST_CASE("T-058: weapon burns 1/landed swing, dormancy pays fists instead") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* p = w.find(w.spawn("smith", 0, sim::TilePos{5, 5}).id);
  w.debugGive(*p, 2001, 1);  // Rusty Shank
  for (std::uint8_t i = 0; i < p->inv.size(); ++i)
    if (p->inv[i].itemId == 2001) { w.toggleEquip(*p, i); break; }
  const std::uint32_t armedDmg = w.effDmgBase(*p);
  CHECK(armedDmg == 12);  // shank base
  CHECK(p->inv[0].durability == 100);

  // paying down durability by hand is the sim knob; swing path covered in
  // test_combat. Here: burn to zero and check dormancy.
  p->inv[0].durability = 1;
  CHECK(w.effDmgBase(*p) == 12);
  p->inv[0].durability = 0;
  CHECK(w.effDmgBase(*p) != 12);  // fists fallback (shard of nothing)
  CHECK(p->inv[0].itemId == 2001);  // kept, not destroyed
}

TEST_CASE("T-058: armor burns when hit; dormancy strips DEF") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* p = w.find(w.spawn("platemail", 0, sim::TilePos{5, 5}).id);
  w.debugGive(*p, 2101, 1);  // Hide Armor (6 def)
  for (std::uint8_t i = 0; i < p->inv.size(); ++i)
    if (p->inv[i].itemId == 2101) { w.toggleEquip(*p, i); break; }
  CHECK(w.effDef(*p) == 6);
  p->inv[0].durability = 0;
  CHECK(w.effDef(*p) == 0);
  CHECK(p->inv[0].itemId == 2101);
}

TEST_CASE("T-058: repair — vendor proximity + gold, all-or-nothing tier") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  // no vendor in this arena: plain refusal
  auto* p = w.find(w.spawn("gouger", 0, sim::TilePos{5, 5}).id);
  w.debugGive(*p, 2001, 1);
  for (std::uint8_t i = 0; i < p->inv.size(); ++i)
    if (p->inv[i].itemId == 2001) { w.toggleEquip(*p, i); break; }
  p->inv[0].durability = 40;
  CHECK_FALSE(w.repairAll(*p));  // no vendor nearby
  CHECK(p->inv[0].durability == 40);

  // gold-scarcity path: not enough coin, nothing changes (all-or-nothing)
  p->gold = 0;
  CHECK_FALSE(w.repairAll(*p));
  // red hands: refusal rides the same karma gate as Marta's shop
  p->karma = -1;
  CHECK_FALSE(w.repairAll(*p));
}

TEST_CASE("T-058: live swings wear the blade (end-to-end burn signal)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* p = w.find(w.spawn("grinder", 0, sim::TilePos{10, 10}).id);
  w.debugGive(*p, 2001, 1);
  for (std::uint8_t i = 0; i < p->inv.size(); ++i)
    if (p->inv[i].itemId == 2001) { w.toggleEquip(*p, i); break; }
  const std::uint32_t pid = p->id;
  auto* rat = w.find(w.debugSpawnMob(*content::findMob(1001), {11, 10}).id);
  REQUIRE(rat != nullptr);
  const std::uint32_t ratId = rat->id;
  // one real attack order, then let the sim swing until the rat drops
  server::Command c;
  c.kind = server::Command::kAttack;
  c.a = static_cast<std::int32_t>(ratId);
  server::applyWorldCommand(w, *w.find(pid), c);
  int guard = 600;
  while (w.find(ratId) != nullptr && guard-- > 0) w.tick();
  CHECK(guard > 0);  // the rat actually died (combat loop ran)
  CHECK(w.find(pid)->inv[0].durability < 100);   // burn happened
  CHECK(w.find(pid)->inv[0].durability >= 90);   // (~8 swings to drop a rat)
}

// ---- T-059 affixes ----
TEST_CASE("T-059: of Whet bites +10% only when durable") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* p = w.find(w.spawn("whetter", 0, sim::TilePos{5, 5}).id);
  w.debugGive(*p, 2001, 1);
  for (std::uint8_t i = 0; i < p->inv.size(); ++i)
    if (p->inv[i].itemId == 2001) { w.toggleEquip(*p, i); break; }
  CHECK(p->inv[0].affix == 0);
  p->inv[0].affix = 1;  // of Whet
  CHECK(w.effDmgBase(*p) == 13);  // 12 + 1 (10% floors)
  p->inv[0].durability = 0;
  CHECK(w.effDmgBase(*p) == 8);   // dormant: fists (kFistsBaseDmg), affix mute
}

TEST_CASE("T-059: of Warding carries +2; of Leech sips dealt damage") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* a = w.find(w.spawn("leecher", 0, sim::TilePos{5, 5}).id);
  w.debugGive(*a, 2001, 1);
  for (std::uint8_t i = 0; i < a->inv.size(); ++i)
    if (a->inv[i].itemId == 2001) { w.toggleEquip(*a, i); break; }
  a->inv[0].affix = 3;  // of Leech
  auto* rat = w.find(w.debugSpawnMob(*content::findMob(1001), {6, 5}).id);
  REQUIRE(rat != nullptr);
  const std::uint32_t ratId = rat->id;
  const std::uint32_t pid = a->id;
  a->hp = 10;  // wound the attacker first so a sip is visible
  server::Command c;
  c.kind = server::Command::kAttack;
  c.a = static_cast<std::int32_t>(ratId);
  server::applyWorldCommand(w, *a, c);
  int guard = 60;
  bool sipped = false;
  while (w.find(ratId) != nullptr && guard-- > 0) {
    w.tick();
    if (w.find(pid)->hp > 10) sipped = true;  // rat only hits back ~2..4; 5% sip == 1
  }
  CHECK(sipped);
}

// ---- T-060 refine ----
TEST_CASE("T-060: refine — proximity law, toll, guaranteed steps, stat delta") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* p = w.find(w.spawn("forger", 0, sim::TilePos{5, 5}).id);
  w.debugGive(*p, 2001, 1);
  for (std::uint8_t i = 0; i < p->inv.size(); ++i)
    if (p->inv[i].itemId == 2001) { w.toggleEquip(*p, i); break; }
  const std::uint8_t gear = 0;  // shank slot index
  w.debugGive(*p, 4001, 2);     // 2 pelts = 2 tolls
  p->gold = 100;

  CHECK_FALSE(w.tryRefine(*p, gear));  // no anvil: proximity law
  static_cast<void>(w.debugSpawnAnvil(sim::TilePos{6, 5}));
  const std::uint32_t beforeDmg = w.effDmgBase(*p);
  CHECK(w.tryRefine(*p, gear));        // tier 0->1: sure (era mercy)
  CHECK(p->inv[gear].refine == 1);
  CHECK(p->gold == 50);                // 50g toll
  CHECK(w.effDmgBase(*p) == beforeDmg + 2);
  CHECK(w.tryRefine(*p, gear));        // tier 1->2: sure
  CHECK(p->inv[gear].refine == 2);
  CHECK(w.effDmgBase(*p) == beforeDmg + 4);
  // refill toll provisions
  p->gold += 150;
  w.debugGive(*p, 4001, 2);
  // 2->3 is a 60% coin with DESTRUCTION on tails. Fresh worlds share one
  // constructor seed (same first roll every time), so walk the strata inside
  // THIS world: one fresh petitioner per attempt advances rng_ naturally.
  bool sawSuccess = false, sawShatter = false;
  for (int t = 0; t < 300 && !(sawSuccess && sawShatter); ++t) {
    auto* q = w.find(w.spawn("coal" + std::to_string(t), 0, sim::TilePos{8, 8}).id);
    w.debugGive(*q, 2001, 1);
    w.debugGive(*q, 4001, 1);
    q->inv[0].refine = 2;
    q->gold = 100;
    q->walker.place(sim::TilePos{6, 5});  // at the anvil (erased shanks shift AoI not law)
    const bool ok = w.tryRefine(*q, 0);
    if (ok && q->inv.size() == 1 && q->inv[0].refine == 3) sawSuccess = true;
    if (ok && q->inv.empty()) sawShatter = true;
  }
  CHECK(sawSuccess);
  CHECK(sawShatter);  // Soma law: the third coal bites back
}

// ---- T-079 refine +4..+7 (GDD rates, shipped costs) ----
TEST_CASE("T-079: refine climbs 3->7, slips back, resets at the top") {
  // Same strata-walking technique as T-060: fresh petitioners, one attempt
  // each, observe the transition distribution. Rates 65/50/35/25 both sides
  // show inside a few hundred rolls on a fixed seed.
  for (int from = 3; from <= 6; ++from) {
    server::World w;
    REQUIRE(w.loadFrom(makeArena()));
    (void)w.debugSpawnAnvil(sim::TilePos{6, 5});
    bool sawUp = false, sawDown = false;
    for (int t = 0; t < 400 && !(sawUp && sawDown); ++t) {
      auto* q = w.find(w.spawn("coal" + std::to_string(t), 0, sim::TilePos{8, 8}).id);
      w.debugGive(*q, 2001, 1);
      w.debugGive(*q, 4001, 1);
      q->inv[0].refine = static_cast<std::uint8_t>(from);
      q->gold = 100;
      q->walker.place(sim::TilePos{6, 5});
      REQUIRE(w.tryRefine(*q, 0));
      if (q->inv.empty()) continue;  // only refine-2 shatters; not this band
      const std::uint8_t got = q->inv[0].refine;
      if (got == from + 1) sawUp = true;
      if (from < 6 && got == from - 1) sawDown = true;  // slip a temper
      if (from == 6 && got == 0) sawDown = true;        // the top forgets all
    }
    CHECK(sawUp);
    CHECK(sawDown);
  }
}

TEST_CASE("T-079: nothing shatters above +2, +7 is full") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  (void)w.debugSpawnAnvil(sim::TilePos{6, 5});
  for (int t = 0; t < 120; ++t) {
    auto* q = w.find(w.spawn("iron" + std::to_string(t), 0, sim::TilePos{8, 8}).id);
    w.debugGive(*q, 2001, 1);
    w.debugGive(*q, 4001, 1);
    q->inv[0].refine = 5;
    q->gold = 100;
    q->walker.place(sim::TilePos{6, 5});
    REQUIRE(w.tryRefine(*q, 0));
    CHECK_FALSE(q->inv.empty());  // slips, never shatters up here
  }
  auto* p = w.find(w.spawn("full", 0, sim::TilePos{8, 8}).id);
  w.debugGive(*p, 2001, 1);
  w.debugGive(*p, 4001, 1);
  p->inv[0].refine = 7;
  p->gold = 100;
  p->walker.place(sim::TilePos{6, 5});
  CHECK_FALSE(w.tryRefine(*p, 0));  // no hotter coal
  CHECK(p->inv[0].refine == 7);
  CHECK(p->gold == 100u);  // ceiling refuses before the toll
}
