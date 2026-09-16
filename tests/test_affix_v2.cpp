// T-126 affix v2: table 3 -> 10 with effect hooks (H3 loot depth, 1/5).
// Pins: table shape, per-affix effects (Ox/Focus/Embers-day+night/Vigil/
// Mending/Thorns-reflect+never-kill/Greed-twin-world), blob round-trip
// for 4..10, 1M-draw fixed-seed distribution.
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

#include "command.h"
#include "content/items.h"
#include "content/mobs.h"
#include "sim/clock.h"
#include "sim/rng.h"
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

// give itemId, stamp affix, THEN equip (live order: affix lands at drop,
// hpMax-class effects materialize at equip) ; returns the slot index.
int giveAffixed(server::World& w, server::Entity& e, std::uint32_t itemId,
                std::uint8_t affix) {
  REQUIRE(w.debugGive(e, itemId, 1));
  for (std::uint8_t i = 0; i < e.inv.size(); ++i) {
    if (e.inv[i].itemId == itemId && !e.inv[i].equipped) {
      e.inv[i].affix = affix;
      REQUIRE(w.toggleEquip(e, i));
      return i;
    }
  }
  REQUIRE(false);
  return -1;
}

sim::Tick tickAtHour(float h) {
  const double fromStart = h - bh::sim::kStartHour;
  const double wrapped = fromStart < 0 ? fromStart + 24.0 : fromStart;
  return static_cast<sim::Tick>(wrapped *
                               static_cast<double>(bh::sim::kTicksPerGameHour));
}
}  // namespace

TEST_CASE("T-126: table holds 10 named affixes") {
  CHECK(content::kAffixCount == 10);
  for (std::uint8_t i = 1; i <= content::kAffixCount; ++i) {
    CHECK(std::string(content::kAffixNames[i]).size() > 0);
  }
  CHECK(std::string(content::kAffixNames[4]) == "of the Ox");
  CHECK(std::string(content::kAffixNames[8]) == "of the Vigil");
  CHECK(std::string(content::kAffixNames[10]) == "of Mending");
}

TEST_CASE("T-126: of the Ox +20 hpMax on equip, released on unequip") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "oxbearer", 5, 5);
  const std::uint32_t base = p->hpMax;
  const int slot = giveAffixed(w, *p, 2101, 4);
  CHECK(p->hpMax == base + 20u);
  REQUIRE(w.toggleEquip(*p, static_cast<std::uint8_t>(slot)));
  CHECK(p->hpMax == base);
}

TEST_CASE("T-126: of Focus +4 ACC on the blade") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "sharpeye", 5, 5);
  const std::uint32_t bare = w.effAcc(*p);  // 2*dex, no bless
  giveAffixed(w, *p, 2001, 6);
  CHECK(w.effAcc(*p) == bare + 4u);
}

TEST_CASE("T-126: of Embers +2 by day, +3 after dark") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "ember", 5, 5);
  giveAffixed(w, *p, 2001, 7);
  w.debugSetTick(tickAtHour(12.0f));
  REQUIRE_FALSE(w.isNight());
  CHECK(w.debugWeaponDmg(*p) == 14u);  // shank 12 + 2
  w.debugSetTick(tickAtHour(22.0f));
  REQUIRE(w.isNight());
  CHECK(w.debugWeaponDmg(*p) == 15u);  // +1 night kicker
}

TEST_CASE("T-126: of the Vigil +2 light while lit") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "vigil", 20, 20);
  giveAffixed(w, *p, 2101, 8);  // vigil on armor: slot-agnostic
  REQUIRE(w.debugGive(*p, 3003, 1));
  std::uint8_t torch = 255;
  for (std::uint8_t s = 0; s < p->inv.size(); ++s)
    if (p->inv[s].itemId == 3003) torch = s;
  REQUIRE(torch != 255);
  REQUIRE(w.useItem(*p, torch));
  CHECK(p->lightRadius == 8u);  // 6 + 2
}

TEST_CASE("T-126: of Mending doubles the OOC regen proc") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* m = spawnP(w, "mending", 5, 5);
  server::Entity* c = spawnP(w, "control", 8, 8);
  giveAffixed(w, *m, 2101, 10);
  REQUIRE(w.debugGive(*c, 2101, 1));  // plain armor, no affix
  m->hp = m->hpMax - 6;
  c->hp = c->hpMax - 6;
  const sim::Tick now = w.tickCount();
  m->lastHurtTick = now - 10000;
  c->lastHurtTick = now - 10000;
  // jump to the next 40-tick proc edge and step onto it
  const sim::Tick edge = (now / 40 + 1) * 40;
  w.debugSetTick(edge - 1);
  const std::uint32_t mBefore = m->hp, cBefore = c->hp;
  w.tick();
  CHECK(w.find(m->id)->hp == mBefore + 2u);
  CHECK(w.find(c->id)->hp == cBefore + 1u);
}

TEST_CASE("T-126: of Thorns reflects 2 per swing and never kills") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* a = spawnP(w, "thorny", 5, 5);
  server::Entity* b = spawnP(w, "striker", 6, 5);
  giveAffixed(w, *a, 2101, 4 + 1);  // affix 5 of Thorns on armor
  REQUIRE(w.debugGive(*b, 2001, 1));
  for (std::uint8_t i = 0; i < b->inv.size(); ++i)
    if (b->inv[i].itemId == 2001) { REQUIRE(w.toggleEquip(*b, i)); break; }
  server::Command c;
  c.kind = server::Command::kAttack;
  c.a = static_cast<std::int32_t>(a->id);
  server::applyWorldCommand(w, *b, c);
  const std::uint32_t bHp0 = b->hp;
  int guard = 400;
  while (w.find(b->id)->hp == bHp0 && guard-- > 0) w.tick();
  CHECK(w.find(b->id)->hp == bHp0 - 2u);  // one reflect observed
  // the never-kill floor: a 2-hp striker keeps swinging and stays alive
  w.find(b->id)->hp = 2;
  guard = 400;
  while (!w.find(a->id)->dead && guard-- > 0) {
    w.tick();
    if (w.find(a->id)->hp < 100) break;  // a second swing landed
  }
  CHECK(w.find(b->id)->hp >= 1u);
  CHECK_FALSE(w.find(b->id)->dead);
}

TEST_CASE("T-126: of Greed +10% kill gold (paired worlds)") {
  // Pair two seed-identical worlds; the ONLY difference is affix 9 on the
  // killer's blade. Same stream => same base roll => exact 110/100.
  auto run = [](bool greed) {
    server::World w;
    w.loadFrom(makeArena());
    server::Entity* p = spawnP(w, greed ? "greedy" : "plain", 5, 5);
    w.debugGive(*p, 2002, 1);  // Pit Blade: rats die fast
    for (std::uint8_t i = 0; i < p->inv.size(); ++i)
      if (p->inv[i].itemId == 2002) {
        w.toggleEquip(*p, i);
        if (greed) p->inv[i].affix = 9;
        break;
      }
    server::Entity* rat =
        w.find(w.debugSpawnMob(*content::findMob(1001), {6, 5}).id);
    const std::uint32_t ratId = rat->id;
    const std::uint32_t pid = p->id;
    const std::uint32_t gold0 = p->gold;
    server::Command c;
    c.kind = server::Command::kAttack;
    c.a = static_cast<std::int32_t>(ratId);
    server::applyWorldCommand(w, *w.find(pid), c);
    int guard = 400;
    while (w.find(ratId) != nullptr && guard-- > 0) w.tick();
    REQUIRE(w.find(ratId) == nullptr);
    return w.find(pid)->gold - gold0;
  };
  const std::uint32_t base = run(false);
  const std::uint32_t greedy = run(true);
  REQUIRE(base >= 6u);  // rat pays 6..14: the roll actually fired
  CHECK(greedy == base * 110u / 100u);
}

TEST_CASE("T-126: blob round-trips affix values 4..10") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  for (std::uint8_t a = 4; a <= 10; ++a) {
    std::vector<server::InvSlot> out;
    const std::string blob =
        "2001:1:1:0:100:" + std::to_string(a) + ":0;";
    server::parseInvBlob(blob, out);
    REQUIRE(out.size() == 1u);
    CHECK(out[0].affix == a);
    const std::string back = server::canonicalInvBlob(out);
    CHECK(back.find(":" + std::to_string(a) + ":") != std::string::npos);
  }
}

TEST_CASE("T-126: 1M fixed-seed rolls spread 1..10") {
  sim::Rng rng(0xC0FFEEULL);
  std::uint32_t buckets[11] = {};
  for (int i = 0; i < 1000000; ++i) {
    const std::int64_t r = rng.range(1, content::kAffixCount);
    REQUIRE(r >= 1);
    REQUIRE(r <= 10);
    ++buckets[static_cast<int>(r)];
  }
  for (int v = 1; v <= 10; ++v) {
    CHECK(buckets[v] > 90000u);
    CHECK(buckets[v] < 110000u);
  }
}
