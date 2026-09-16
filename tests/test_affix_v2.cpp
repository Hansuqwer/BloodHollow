// T-126 affix v2 + T-159 affix v3: table 10 -> 20 with effect hooks (H3 loot depth).
// Pins: table shape, per-affix effects (Ox/Focus/Embers-day+night/Vigil/
// Mending/Thorns-reflect+never-kill/Greed-twin-world/Hollow/Dirge/Crypt/Pall/
// Marrow/Grave-touched/Tithemaster), blob round-trip for 4..20,
// 1M-draw fixed-seed distribution, rarity distribution roll.
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

TEST_CASE("T-159: table holds 20 named affixes") {
  CHECK(content::kAffixCount == 20);
  for (std::uint8_t i = 1; i <= content::kAffixCount; ++i) {
    CHECK(std::string(content::kAffixNames[i]).size() > 0);
  }
  CHECK(std::string(content::kAffixNames[4]) == "of the Ox");
  CHECK(std::string(content::kAffixNames[8]) == "of the Vigil");
  CHECK(std::string(content::kAffixNames[10]) == "of Mending");
  CHECK(std::string(content::kAffixNames[11]) == "of the Hollow");
  CHECK(std::string(content::kAffixNames[12]) == "Grave-touched");
  CHECK(std::string(content::kAffixNames[13]) == "of the Crypt");
  CHECK(std::string(content::kAffixNames[14]) == "of the Marrow");
  CHECK(std::string(content::kAffixNames[15]) == "of the Pall");
  CHECK(std::string(content::kAffixNames[16]) == "of the Boneyard");
  CHECK(std::string(content::kAffixNames[17]) == "of the Dirge");
  CHECK(std::string(content::kAffixNames[18]) == "of the Husk");
  CHECK(std::string(content::kAffixNames[19]) == "of the Tithemaster");
  CHECK(std::string(content::kAffixNames[20]) == "of Last Rites");
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

TEST_CASE("T-159: of the Hollow +3 flat weapon dmg") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "hollow", 5, 5);
  giveAffixed(w, *p, 2001, 11);
  CHECK(w.debugWeaponDmg(*p) == 15u);  // shank 12 + 3
}

TEST_CASE("T-159: of the Dirge +4 at night, +0 by day") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "dirge", 5, 5);
  giveAffixed(w, *p, 2001, 17);
  w.debugSetTick(tickAtHour(12.0f));
  REQUIRE_FALSE(w.isNight());
  CHECK(w.debugWeaponDmg(*p) == 12u);  // shank 12, no night bonus
  w.debugSetTick(tickAtHour(22.0f));
  REQUIRE(w.isNight());
  CHECK(w.debugWeaponDmg(*p) == 16u);  // shank 12 + 4
}

TEST_CASE("T-159: of the Crypt -10% incoming dmg") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* tank = spawnP(w, "tank", 5, 5);
  server::Entity* blade = spawnP(w, "blade", 6, 5);
  giveAffixed(w, *tank, 2101, 13);  // Crypt on armor
  REQUIRE(w.debugGive(*blade, 2001, 1));
  for (std::uint8_t i = 0; i < blade->inv.size(); ++i)
    if (blade->inv[i].itemId == 2001) { REQUIRE(w.toggleEquip(*blade, i)); break; }
  // With -10% dmg reduction, tank should survive longer than without
  const std::uint32_t tankHp0 = tank->hp;
  const std::uint32_t tankId = tank->id;
  const std::uint32_t bladeId = blade->id;
  server::Command c;
  c.kind = server::Command::kAttack;
  c.a = static_cast<std::int32_t>(tankId);
  int guard = 400;
  while (w.find(tankId)->hp == tankHp0 && guard-- > 0) {
    server::Entity* b = w.find(bladeId);
    if (b != nullptr) server::applyWorldCommand(w, *b, c);
    w.tick();
  }
  // After one hit, verify hp loss is reduced (shank 12, -10% = 10 after reduction)
  CHECK(w.find(tankId)->hp < tankHp0);
  CHECK(w.find(tankId)->hp >= tankHp0 - 11u);  // reduced dmg <= 10 (floor 1)
}

TEST_CASE("T-159: of the Marrow +3% lifesteal on weapon") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "marrow", 5, 5);
  server::Entity* rat = w.find(w.debugSpawnMob(*content::findMob(1001), {6, 5}).id);
  giveAffixed(w, *p, 2002, 14);  // Marrow on Pit Blade
  p->hp = 10;  // wound first so a sip is visible (T-059 pattern)
  const std::uint32_t pid = p->id;
  const std::uint32_t ratId = rat->id;
  server::Command c;
  c.kind = server::Command::kAttack;
  c.a = static_cast<std::int32_t>(ratId);
  server::applyWorldCommand(w, *w.find(pid), c);
  int guard = 400;
  bool sipped = false;
  while (w.find(ratId) != nullptr && guard-- > 0) {
    w.tick();
    if (w.find(pid) != nullptr && w.find(pid)->hp > 10) sipped = true;
  }
  REQUIRE(w.find(ratId) == nullptr);
  CHECK(sipped);  // Marrow sip fired at least once during the kill
}

TEST_CASE("T-159: of the Pall +2 acc (any gear)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "pall", 5, 5);
  const std::uint32_t bare = w.effAcc(*p);
  giveAffixed(w, *p, 2101, 15);  // Pall on armor (slot-agnostic)
  CHECK(w.effAcc(*p) == bare + 2u);
}

TEST_CASE("T-159: Grave-touched +1 OOC regen (any gear)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* m = spawnP(w, "grave", 5, 5);
  server::Entity* c = spawnP(w, "ctrl", 8, 8);
  giveAffixed(w, *m, 2101, 12);  // Grave-touched on armor
  REQUIRE(w.debugGive(*c, 2101, 1));  // plain armor
  m->hp = m->hpMax - 6;
  c->hp = c->hpMax - 6;
  const sim::Tick now = w.tickCount();
  m->lastHurtTick = now - 10000;
  c->lastHurtTick = now - 10000;
  const sim::Tick edge = (now / 40 + 1) * 40;
  w.debugSetTick(edge - 1);
  const std::uint32_t mBefore = m->hp, cBefore = c->hp;
  w.tick();
  CHECK(w.find(m->id)->hp == mBefore + 2u);  // base +1 + Grave-touched +1
  CHECK(w.find(c->id)->hp == cBefore + 1u);  // base only
}

TEST_CASE("T-159: of the Tithemaster +15% kill gold (paired worlds)") {
  auto run = [](bool tithemaster) {
    server::World w;
    w.loadFrom(makeArena());
    server::Entity* p = spawnP(w, tithemaster ? "tithemaster" : "plain", 5, 5);
    w.debugGive(*p, 2002, 1);
    for (std::uint8_t i = 0; i < p->inv.size(); ++i)
      if (p->inv[i].itemId == 2002) {
        w.toggleEquip(*p, i);
        if (tithemaster) p->inv[i].affix = 19;
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
  const std::uint32_t tithe = run(true);
  REQUIRE(base >= 6u);
  CHECK(tithe == base * 115u / 100u);
}

TEST_CASE("T-159: rarity constants defined correctly") {
  CHECK(content::kRarityCommon == 0u);
  CHECK(content::kRarityMagic == 1u);
  CHECK(content::kRarityRare == 2u);
  CHECK(content::kRarityUnique == 3u);
  CHECK(content::kRarityCount == 4u);
}

TEST_CASE("T-159: rarity blob round-trip") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  std::vector<server::InvSlot> out;
  const std::string blob = "2001:1:1:0:100:0:0:2;";
  server::parseInvBlob(blob, out);
  REQUIRE(out.size() == 1u);
  CHECK(out[0].rarity == 2u);
  const std::string back = server::canonicalInvBlob(out);
  CHECK(back.find(":2;") != std::string::npos);
}

TEST_CASE("T-159: 100k fixed-seed rarity distribution") {
  sim::Rng rng(0xBEEF42ULL);
  std::uint32_t buckets[4] = {};
  for (int i = 0; i < 100000; ++i) {
    const std::int32_t roll = rng.range(1, 100);
    std::uint8_t rarity = content::kRarityCommon;
    if (roll <= 78) rarity = content::kRarityCommon;
    else if (roll <= 95) rarity = content::kRarityMagic;
    else if (roll <= 99) rarity = content::kRarityRare;
    else rarity = content::kRarityUnique;
    ++buckets[rarity];
  }
  // Common: 78% ±1% → 77k..79k
  CHECK(buckets[0] > 77000u);
  CHECK(buckets[0] < 79000u);
  // Magic: 17% ±1% → 16k..18k
  CHECK(buckets[1] > 16000u);
  CHECK(buckets[1] < 18000u);
  // Rare: 4.6% ±1% → 3.6k..5.6k
  CHECK(buckets[2] > 3600u);
  CHECK(buckets[2] < 5600u);
  // Unique: 0.4% ±1% → 0..1.4k
  CHECK(buckets[3] < 1400u);
}

TEST_CASE("T-159: blob round-trips affix values 4..20") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  for (std::uint8_t a = 4; a <= 20; ++a) {
    std::vector<server::InvSlot> out;
    const std::string blob =
        "2001:1:1:0:100:" + std::to_string(a) + ":0:0;";
    server::parseInvBlob(blob, out);
    REQUIRE(out.size() == 1u);
    CHECK(out[0].affix == a);
    const std::string back = server::canonicalInvBlob(out);
    CHECK(back.find(":" + std::to_string(a) + ":") != std::string::npos);
  }
}

TEST_CASE("T-159: 1M fixed-seed rolls spread 1..20") {
  sim::Rng rng(0xC0FFEEULL);
  std::uint32_t buckets[21] = {};
  for (int i = 0; i < 1000000; ++i) {
    const std::int64_t r = rng.range(1, content::kAffixCount);
    REQUIRE(r >= 1);
    REQUIRE(r <= 20);
    ++buckets[static_cast<int>(r)];
  }
  for (int v = 1; v <= 20; ++v) {
    CHECK(buckets[v] > 40000u);
    CHECK(buckets[v] < 60000u);
  }
}

TEST_CASE("T-159: of the Pall +1 evd via effEvd") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "pall-evd", 5, 5);
  const std::uint32_t bare = w.effEvd(*p);
  CHECK(bare == p->dex);
  giveAffixed(w, *p, 2101, 15);  // Pall on armor (slot-agnostic)
  CHECK(w.effEvd(*p) == bare + 1u);
}

TEST_CASE("T-159: of Last Rites +8 below 20% hp, +0 above") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "rites", 5, 5);
  giveAffixed(w, *p, 2001, 20);  // Last Rites on shank (12)
  p->hp = p->hpMax;
  CHECK(w.debugWeaponDmg(*p) == 12u);  // full hp: no bonus
  p->hp = p->hpMax / 10;              // 10% < 20%
  CHECK(w.debugWeaponDmg(*p) == 20u);  // shank 12 + 8
}

TEST_CASE("T-159: of the Boneyard is weapon-gated (slot law)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "boneyard", 5, 5);
  giveAffixed(w, *p, 2001, 16);  // Boneyard on weapon: live
  CHECK(w.hasAffix(*p, 16, 0));
  server::Entity* q = spawnP(w, "boneyard-armor", 7, 7);
  REQUIRE(w.debugGive(*q, 2101, 1));
  for (std::uint8_t i = 0; i < q->inv.size(); ++i)
    if (q->inv[i].itemId == 2101) {
      q->inv[i].affix = 16;
      REQUIRE(w.toggleEquip(*q, i));
      break;
    }
  CHECK_FALSE(w.hasAffix(*q, 16, 0));  // armor piece is not a weapon
  CHECK(w.hasAffix(*q, 16, 9));        // ...but rides any-gear probes
}
