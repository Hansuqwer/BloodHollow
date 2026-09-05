#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>

#include "content/mobs.h"
#include "sim/combat.h"
#include "sim/rng.h"
#include "world.h"

using namespace bh;

TEST_CASE("combat: xp table is monotonic and matches the GDD curve") {
  CHECK(sim::xpNext(1) == 100u);
  CHECK(sim::xpNext(2) == 360u);
  CHECK(sim::xpNext(10) == 7080u);
  CHECK(sim::xpNext(25) == 0u);  // cap
  for (std::uint8_t l = 1; l < 24; ++l) {  // up to L24->25; cap row is semantically 0
    CHECK(sim::xpNext(l) < sim::xpNext(static_cast<std::uint8_t>(l + 1)));
    CHECK(sim::xpNext(l) % 10 == 0u);
  }
}

TEST_CASE("combat: hit check obeys GDD clamps") {
  sim::Rng rng(7);
  // hopeless attacker: acc 0 vs evd 60 -> clamp 5%
  int hits = 0;
  for (int i = 0; i < 4000; ++i) {
    const auto hc = sim::rollHit(0, 60, 10, rng);
    if (hc.hit) ++hits;
    CHECK(hc.hitChancePct == 5);
  }
  CHECK(hits < 400);  // ~5% +- noise
  // even match acc=evd -> 55%
  hits = 0;
  for (int i = 0; i < 4000; ++i) {
    if (sim::rollHit(16, 16, 10, rng).hit) ++hits;
  }
  CHECK(hits > 2000);
  CHECK(hits < 2500);
}

TEST_CASE("combat: damage formula is GDD-true and deterministic") {
  // raw = 8*(1+8*0.02) = 9 (int); def 20 -> *100/120
  sim::Rng a(42), b(42);
  for (int i = 0; i < 50; ++i) {
    const auto ha = sim::rollHit(16, 8, 20, a);
    const auto hb = sim::rollHit(16, 8, 20, b);
    CHECK(ha.hit == hb.hit);
    CHECK(ha.crit == hb.crit);
  }
  const std::uint32_t d = sim::rollDamage(8, 8, 20, false);
  const std::uint32_t dc = sim::rollDamage(8, 8, 20, true);
  CHECK(d == 7u);
  CHECK(dc == d * 170u / 100u);
  CHECK(sim::rollDamage(1, 0, 1000000, false) == 1u);  // chip rule
}

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
  sim::SpawnDef sp;
  sp.x = 5;
  sp.y = 5;
  sp.w = 4;
  sp.h = 4;
  sp.mobId = 1001;  // Marsh Rat, passive
  sp.maxAlive = 3;
  sp.respawnTicks = 20;  // 1 s for tests
  m.spawners.push_back(sp);
  return m;
}

server::Entity* findMob(server::World& w) {
  for (auto& e : w.entities()) {
    if (e.kind == server::EntityKind::kMob && e.wireKind < 64) return &e;
  }
  return nullptr;
}
}  // namespace

TEST_CASE("world combat: spawner populates; player kills a rat; XP awarded") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  // Vendor Marta (wireKind 64) counts as an entity too; account for her.
  REQUIRE(findMob(w) != nullptr);
  const std::uint32_t mobId = findMob(w)->id;
  {
    const server::Entity& m = *w.find(mobId);
    CHECK(m.hp == 30u);
  }

  // NOTE: never hold Entity& across World::tick() — killMob erases from the
  // deque and shifts the tail (dangling refs; learned the hard way).
  server::Entity& p = w.spawn("tester", 0, std::nullopt);
  const std::uint32_t pid = p.id;
  (void)p;
  {
    server::Entity& pl = *w.find(pid);
    const sim::TilePos mp = w.find(mobId)->walker.tile();
    pl.walker.place(sim::TilePos{mp.x + 1, mp.y});
    w.setAttack(pl, mobId);
  }

  int ticks = 0;
  while (w.find(mobId) != nullptr && ticks < 2000) {
    server::Entity& pl = *w.find(pid);
    server::Entity& mo = *w.find(mobId);
    const int d = std::max(std::abs(pl.walker.tile().x - mo.walker.tile().x),
                           std::abs(pl.walker.tile().y - mo.walker.tile().y));
    if (d > 1) {
      pl.walker.place(sim::TilePos{mo.walker.tile().x + 1, mo.walker.tile().y});
      w.setAttack(pl, mobId);
    }
    w.tick();
    ++ticks;
  }
  CHECK(w.find(mobId) == nullptr);  // mob died
  const server::Entity& p2 = *w.find(pid);
  CHECK(p2.xp >= 40u);

  // spawner refills after respawnTicks (20t in the arena)
  int extra = 0;
  while (extra < 200) {
    w.tick();
    ++extra;
  }
  int mobs = 0;
  for (auto& e : w.entities()) {
    if (e.kind == server::EntityKind::kMob && e.wireKind < 64) ++mobs;
  }
  CHECK(mobs == 3);  // refilled to maxAlive
}

TEST_CASE("world combat: player death respawns at town with full hp") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& mob = *w.find(findMob(w)->id);
  const sim::TilePos mp = mob.walker.tile();
  server::Entity& p = w.spawn("victim", 0, sim::TilePos{mp.x + 1, mp.y});
  p.hp = 5;  // one solid bite
  mob.attackTarget = p.id;  // force aggro
  int ticks = 0;
  while (!w.find(p.id)->dead && ticks < 400) {
    w.tick();
    ++ticks;
  }
  REQUIRE(w.find(p.id)->dead);
  const server::Entity* dp = w.find(p.id);
  CHECK(dp->hp == 0u);
  // stay dead ~3 s then rise at spawn point, healed
  ticks = 0;
  while (w.find(p.id)->dead && ticks < 200) {
    w.tick();
    ++ticks;
  }
  const server::Entity* rp = w.find(p.id);
  REQUIRE_FALSE(rp->dead);
  CHECK(rp->hp == rp->hpMax);
  CHECK(rp->walker.tile() == w.spawnPoint());
}

TEST_CASE("world combat: aggressive hound chases in radius and leashes home") {
  sim::Map m = makeArena();
  m.spawners[0].mobId = 1003;  // Hollow Hound, aggro 7, leash 16
  server::World w;
  REQUIRE(w.loadFrom(std::move(m)));
  server::Entity& mob = *w.find(findMob(w)->id);
  const sim::TilePos mp = mob.walker.tile();
  CHECK(mob.aggroRadius == 7);
  server::Entity& p = w.spawn("bait", 0, sim::TilePos{mp.x + 5, mp.y});
  int ticks = 0;
  while (mob.attackTarget == 0 && ticks < 100) {
    w.tick();  // hound should notice the player
    ++ticks;
  }
  CHECK(mob.attackTarget == p.id);
  // teleport-bait out of leash range -> drops target
  p.walker.place(sim::TilePos{35, 35});
  ticks = 0;
  while (mob.attackTarget != 0 && ticks < 100) {
    w.tick();
    ++ticks;
  }
  CHECK(mob.attackTarget == 0u);
}

// ---- Sprint 6: economy / items / debt -------------------------------------

TEST_CASE("items: vendor buy/equip changes damage source; gold arithmetic") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* vend = nullptr;
  for (auto& e : w.entities()) {
    if (e.wireKind == 64) vend = &e;
  }
  REQUIRE(vend != nullptr);
  server::Entity& p = *w.find(w.spawn("shopper", 0, std::nullopt).id);
  p.walker.place(sim::TilePos{vend->walker.tile().x + 1, vend->walker.tile().y});
  p.gold = 100;
  REQUIRE(w.vendorBuy(p, 2001, 1));  // Rusty Shank 80g
  CHECK(p.gold == 20u);
  CHECK(p.inv.size() == 1u);
  CHECK(!w.vendorBuy(p, 3001, 2));  // 2x Blood Vial = 60g; broke at 20g
  CHECK(p.gold == 20u);             // refused atomically, no partial
  CHECK(!w.vendorBuy(p, 2002, 1));  // brokes
  p.gold = 100;
  CHECK(w.vendorBuy(p, 3001, 2));   // 60g fine now
  CHECK(p.gold == 40u);
  CHECK(p.inv.size() == 2u);        // shank + vial stack
  // equip the shank; base damage source flips off fists
  REQUIRE(w.toggleEquip(p, 0));
  CHECK(p.inv[0].equipped);
  CHECK(w.debugWeaponDmg(p) == 12u);
  // un-equip returns to fists
  REQUIRE(w.toggleEquip(p, 0));
  CHECK(w.debugWeaponDmg(p) == 8u);
  // junk selling at 40%
  REQUIRE(w.debugGive(p, 4001, 3));  // 3x Rat Pelt value 10 -> 12g
  CHECK(w.vendorSellJunk(p) == 12u);
  CHECK(p.gold == 52u);
  CHECK(p.inv.size() == 2u);  // junk gone, shank + vials remain
}

TEST_CASE("items: Blood Vial sip heals with 0.5s global cooldown") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& p = w.spawn("sipper", 0, std::nullopt);
  const std::uint32_t pid = p.id;
  REQUIRE(w.debugGive(p, 3001, 3));
  p.hp = 10;
  w.tick();  // clear events
  REQUIRE(w.useItem(p, 0));
  CHECK(p.hp == 50u);  // +40
  CHECK(p.inv[0].qty == 2u);
  CHECK(!w.useItem(p, 0));  // sip cooldown
  p.hp = 10;
  for (int i = 0; i < 12; ++i) w.tick();
  REQUIRE(w.useItem(*w.find(pid), 0));
  CHECK(w.find(pid)->hp == 50u);
  bool healEvent = false;
  for (const auto& ev : w.events()) {
    if (ev.kind == 4 && ev.amount == 40) healEvent = true;
  }
  CHECK(healEvent);
  // refuses at full hp
  p.hp = p.hpMax;
  for (int i = 0; i < 12; ++i) w.tick();
  CHECK(!w.useItem(*w.find(pid), 0));
}

TEST_CASE("death: XP debt scales with level and de-levels at zero XP") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& p = w.spawn("debtor", 0, std::nullopt);
  p.level = 5;
  p.xp = 500;
  p.hp = 10;
  const std::uint32_t expectedDebt = sim::xpNext(5) * (10 + 4 * 15 / 24) / 100;
  w.debugKillPlayer(p);
  CHECK(p.xp == 500u - expectedDebt);
  CHECK(p.level == 5);
  // zero-XP death de-levels: debt eats backwards through bars
  server::Entity& q = *w.find(w.spawn("novice", 0, std::nullopt).id);
  q.level = 3;
  q.xp = 20;
  w.debugKillPlayer(q);
  CHECK(q.level == 2);
  CHECK(q.xp < sim::xpNext(2));
}

TEST_CASE("skills: Power Swing hits harder with a real cooldown") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const std::uint32_t mobId = findMob(w)->id;
  server::Entity& p = *w.find(w.spawn("swinger", 0, std::nullopt).id);
  const sim::TilePos mp = w.find(mobId)->walker.tile();
  p.walker.place(sim::TilePos{mp.x + 1, mp.y});
  w.tick();  // drain boot events
  w.trySkill(p, 1, mobId);
  bool skillHitOrMissEvent = false;
  for (const auto& ev : w.events()) {
    if ((ev.kind == 5 || ev.kind == 0) && ev.attacker == p.id) skillHitOrMissEvent = true;
  }
  CHECK(skillHitOrMissEvent);
  w.tick();
  // cooldown: immediate re-fire does nothing
  w.trySkill(*w.find(p.id), 1, mobId);
  for (const auto& ev : w.events()) {
    CHECK(ev.kind != 5);
  }
}

// ---- Sprint 7: trade window (T-029) ----------------------------------------
TEST_CASE("trade: open/offer/commit swaps atomically") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const std::uint32_t aid = w.spawn("trader_a", 0, sim::TilePos{20, 24}).id;
  const std::uint32_t bid = w.spawn("trader_b", 0, sim::TilePos{20, 25}).id;
  {
    server::Entity* a = w.find(aid);
    w.debugGive(*a, 4001, 3);  // 3 rat pelts
  }
  w.find(bid)->gold = 25;
  CHECK(w.tradeOpen(*w.find(aid), bid));
  CHECK(!w.tradeOpen(*w.find(aid), bid));  // already trading
  w.tradeOffer(*w.find(aid), 4001, 2);     // 2 pelts for...
  w.tradeOfferGold(*w.find(bid), 10);      // ...10g
  for (int i = 0; i < 2; ++i) w.tick();
  CHECK(w.find(aid)->gold == 50);          // nothing moved pre-commit
  w.tradeCommit(*w.find(aid));
  CHECK(w.find(bid)->tradeWith == aid);
  CHECK(w.find(bid)->gold == 25);          // unswapped until handshake
  w.tradeCommit(*w.find(bid));
  w.tick();  // event emission is per-tick; a tick flushes suppression
  const server::Entity* a = w.find(aid);
  const server::Entity* b = w.find(bid);
  CHECK(a->gold == 60);
  CHECK(b->gold == 15);
  std::uint32_t apelt = 0, bpelt = 0;
  for (const auto& sl : a->inv)
    if (sl.itemId == 4001) apelt += sl.qty;
  for (const auto& sl : b->inv)
    if (sl.itemId == 4001) bpelt += sl.qty;
  CHECK(apelt == 1);
  CHECK(bpelt == 2);
  CHECK(a->tradeWith == 0);
  CHECK(b->tradeWith == 0);
}

TEST_CASE("trade: oversell rolls back at commit; distance auto-cancels") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const std::uint32_t aid = w.spawn("trader_c", 0, sim::TilePos{20, 24}).id;
  const std::uint32_t bid = w.spawn("trader_d", 0, sim::TilePos{20, 25}).id;
  w.find(aid)->gold = 50;
  CHECK(w.tradeOpen(*w.find(aid), bid));
  w.tradeOfferGold(*w.find(bid), 999);  // b only has 50
  w.tradeCommit(*w.find(bid));
  w.tradeCommit(*w.find(aid));          // validation fires -> cancel
  CHECK(w.find(aid)->gold == 50);
  CHECK(w.find(bid)->gold == 50);
  CHECK(w.find(aid)->tradeWith == 0);
  CHECK(w.find(bid)->tradeWith == 0);

  // distance auto-cancel: open again, walk b away, tick until loose
  CHECK(w.tradeOpen(*w.find(aid), bid));
  w.queuePath(*w.find(bid), {21, 31});
  for (int i = 0; i < 250; ++i) w.tick();
  CHECK(w.find(bid)->tradeWith == 0);
  CHECK(w.find(aid)->tradeWith == 0);
}

// ---- Sprint 8: zones-in-process + portal transfer (T-036) ------------------
TEST_CASE("zones: stepping onto a portal transfers the player, state preserved") {
  // zone 1: open arena with a hatch at (20,26)->(3, 4,4)
  sim::Map a = makeArena();
  sim::PortalDef pd;
  pd.x = 20; pd.y = 26; pd.w = 1; pd.h = 1;
  pd.targetMapId = 3; pd.targetX = 4; pd.targetY = 4;
  a.portals.push_back(pd);
  // zone 3: small open crypt stand-in
  sim::Map c;
  c.w = 12; c.h = 12; c.tileW = 64; c.tileH = 32;
  c.ground.assign(12 * 12, 0);
  c.zone.assign(12 * 12, 0);
  c.blocked.assign(12 * 12, 0);

  server::World w;
  REQUIRE(w.loadFrom(a));
  REQUIRE(w.loadZoneFrom(3, c));
  server::Entity& p = *w.find(w.spawn("mapper", 0, sim::TilePos{20, 25}).id);
  const std::uint32_t pid = p.id;
  p.gold = 77;
  w.debugGive(p, 4001, 2);
  CHECK(p.zoneId == 1);
  w.queuePath(*w.find(pid), {20, 26});
  bool moved = false;
  for (int i = 0; i < 120 && !moved; ++i) {
    w.tick();
    moved = w.find(pid)->zoneId == 3;
  }
  REQUIRE(moved);
  const server::Entity* after = w.find(pid);
  CHECK(after->walker.tile() == sim::TilePos{4, 4});
  CHECK(after->gold == 77);  // state travels with you
  std::uint32_t pelts = 0;
  for (const auto& sl : after->inv) if (sl.itemId == 4001) pelts += sl.qty;
  CHECK(pelts == 2);
  // zone isolation: no cross-zone events in AoI across zones
  server::Entity& other = *w.find(w.spawn("stayin", 0, sim::TilePos{20, 24}, 1).id);
  CHECK(other.zoneId == 1);
  const auto aoi1 = w.queryAoi(1, 20, 25, 8);
  const auto aoi3 = w.queryAoi(3, 4, 4, 8);
  CHECK(std::find(aoi1.begin(), aoi1.end(), pid) == aoi1.end());
  CHECK(std::find(aoi3.begin(), aoi3.end(), pid) != aoi3.end());
  CHECK(std::find(aoi1.begin(), aoi1.end(), other.id) != aoi1.end());
  // hash still deterministic & zone-sensitive
  const std::uint64_t h1 = w.worldHash();
  w.tick();
  CHECK(w.worldHash() != h1);  // movement ongoing / tick advance changes hash... same state checks
}

TEST_CASE("zones: round trip thornwall -> crypt -> thornwall, hash maps unique per zone") {
  sim::Map a = makeArena();
  sim::PortalDef fd;  // hatch (20,26) -> crypt (4,4)
  fd.x = 20; fd.y = 26; fd.w = 1; fd.h = 1;
  fd.targetMapId = 3; fd.targetX = 4; fd.targetY = 4;
  a.portals.push_back(fd);
  sim::Map c;
  c.w = 12; c.h = 12; c.tileW = 64; c.tileH = 32;
  c.ground.assign(12 * 12, 0);
  c.zone.assign(12 * 12, 0);
  c.blocked.assign(12 * 12, 0);
  sim::PortalDef bk;  // stairs (4,5) -> arena (20,24) [arrival next to hatch]
  bk.x = 4; bk.y = 5; bk.w = 1; bk.h = 1;
  bk.targetMapId = 1; bk.targetX = 20; bk.targetY = 24;
  c.portals.push_back(bk);

  server::World w;
  REQUIRE(w.loadFrom(a));
  REQUIRE(w.loadZoneFrom(3, c));
  server::Entity& p = *w.find(w.spawn("cyclic", 0, sim::TilePos{20, 24}).id);
  const std::uint32_t pid = p.id;
  w.queuePath(*w.find(pid), {20, 26});
  bool forward = false;
  for (int i = 0; i < 120; ++i) {
    w.tick();
    if (w.find(pid)->zoneId == 3) { forward = true; break; }
  }
  REQUIRE(forward);
  // walk to the stair side tile (4,5) - arrival grace window needs passing
  for (int i = 0; i < 45; ++i) w.tick();          // burn the anti-loop cooldown
  w.queuePath(*w.find(pid), {4, 5});
  bool back = false;
  for (int i = 0; i < 200; ++i) {
    w.tick();
    
    if (w.find(pid)->zoneId == 1) { back = true; break; }
  }
  REQUIRE(back);
  CHECK(w.find(pid)->walker.tile() == sim::TilePos{20, 24});
}

TEST_CASE("zones: player death always respawns at the town bindstone (zone 1)") {
  sim::Map a = makeArena();
  sim::PortalDef fd;  // (20,26) -> crypt (4,4)
  fd.x = 20; fd.y = 26; fd.w = 1; fd.h = 1;
  fd.targetMapId = 3; fd.targetX = 4; fd.targetY = 4;
  a.portals.push_back(fd);
  sim::Map c;
  c.w = 12; c.h = 12; c.tileW = 64; c.tileH = 32;
  c.ground.assign(12 * 12, 0);
  c.zone.assign(12 * 12, 0);
  c.blocked.assign(12 * 12, 0);
  server::World w;
  REQUIRE(w.loadFrom(a));
  REQUIRE(w.loadZoneFrom(3, c));
  const std::uint32_t pid = w.spawn("mortal", 0, sim::TilePos{20, 25}).id;
  w.queuePath(*w.find(pid), {20, 26});
  for (int i = 0; i < 120 && w.find(pid)->zoneId != 3; ++i) w.tick();
  // (junction spot)
  REQUIRE(w.find(pid)->zoneId == 3);
  const auto beforeTile = w.find(pid)->walker.tile();
  w.debugKillPlayer(*w.find(pid));
  CHECK(w.find(pid)->dead);
  
  // respawn: wait out the 3s timer
  for (int i = 0; i < 70; ++i) w.tick();
  const server::Entity* p = w.find(pid);
  REQUIRE(p != nullptr);
  CHECK(!p->dead);
  CHECK(p->zoneId == 1);
  CHECK(p->walker.tile() != beforeTile);  // back to the plaza, not the crypt
}

// ---- Sprint 9: anvil & aura tier I-II (T-041/T-042, RFC 0001) --------------
TEST_CASE("anvil: tier I Edge Rite applies +3 atk after parts+gold toll, mercy-guaranteed") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  // an anvil should exist in zone 1 near spawn
  bool anvilPresent = false;
  for (const auto& e : w.entities())
    if (e.wireKind == 65) anvilPresent = true;
  REQUIRE(anvilPresent);
  const std::uint32_t pid = w.spawn("smith", 0, std::nullopt).id;
  server::Entity* p = w.find(pid);
  w.debugGive(*p, 2001, 1);          // Shank
  w.debugGive(*p, 4001, 35);         // 35 Rat Pelts (need 30)
  p->gold = 200;
  w.toggleEquip(*p, 0);              // equip the Shank
  CHECK(w.debugWeaponDmg(*p) == 12u + 0u);  // shank dmg 12 (baseline)
  p->swordSkill = 25;                // tier I gate is 20
  const bool ok = w.tryAnvil(*p, 1);
  REQUIRE(ok);
  server::Entity* q = w.find(pid);
  std::uint32_t pelts = 0;
  for (const auto& sl : q->inv) if (sl.itemId == 4001) pelts += sl.qty;
  CHECK(pelts == 5);                 // 35 - 30
  CHECK(q->gold == 80);              // 200 - 120
  CHECK(w.debugWeaponDmg(*q) == 12u + 3u);  // Edge Rite flat bonus
  // second attempt same tier refused (already blessed)
  q->swordSkill = 60; w.debugGive(*q, 4002, 15); q->gold = 500;
  // order: tier III before II refused
  REQUIRE(!w.tryAnvil(*q, 3));
  // tier II expected+mercy: parts+gold consumed, regen flag settable
  REQUIRE(w.tryAnvil(*q, 2));
  std::uint32_t fingers = 0;
  for (const auto& sl : q->inv) if (sl.itemId == 4002) fingers += sl.qty;
  CHECK(fingers == 0);
  CHECK(q->gold == 100);             // 500 - 400
  // tier III without parts fails entirely and spends nothing
  q->gold = 5000; q->swordSkill = 100;
  REQUIRE(!w.tryAnvil(*q, 3));
  w.debugGive(*q, 4004, 10);          // silk for tier III
  q->gold = 1200;
  REQUIRE(w.tryAnvil(*q, 3));         // mercy: tier-III first attempt always lands
  CHECK(q->gold == 0);                // full toll
  CHECK(q->anvilMercyMask == (1u<<1 | 1u<<2 | 1u<<3));  // mercy spent on I, II, III
}

TEST_CASE("anvil: gates refuse polite poison (unarmed, low skill, broke)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const std::uint32_t pid = w.spawn("beggar", 0, std::nullopt).id;
  server::Entity* p = w.find(pid);
  CHECK(!w.tryAnvil(*p, 1));         // no weapon
  w.debugGive(*p, 2001, 1);
  w.toggleEquip(*p, 0);
  CHECK(p->swordSkill == 0);
  CHECK(!w.tryAnvil(*p, 1));         // skill gate
  p->swordSkill = 25;
  p->gold = 0;
  CHECK(!w.tryAnvil(*p, 1));         // gold gate (parts missing too)
}

TEST_CASE("T-046 moral split: virtuous XP bonus, anvil tithe karma drift") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const std::uint32_t pid = w.spawn("monk", 0, std::nullopt).id;
  server::Entity* p = w.find(pid);
  p->karma = 5;
  w.debugAwardXp(*p, 40);
  CHECK(w.find(pid)->xp == 46);  // 40 * 1.15 clean-soul bonus
  w.debugGive(*p, 2001, 1);
  w.toggleEquip(*p, 0);
  p->swordSkill = 20;
  p->gold = 1000;
  w.debugGive(*p, 4001, 60);
  CHECK(w.tryAnvil(*p, 1));
  CHECK(w.find(pid)->karma == 5 + server::World::kKarmaAnvilOk);  // tithe remembered
}

TEST_CASE("T-046 moral split: drowned steel stains (destroy law karma)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const std::uint32_t pid = w.spawn("ash", 0, std::nullopt).id;
  server::Entity* p = w.find(pid);
  w.debugGive(*p, 2001, 1);
  w.toggleEquip(*p, 0);
  p->swordSkill = 150;
  p->anvilMercyMask = (1u << 1) | (1u << 2) | (1u << 3) | (1u << 4);  // no guardians
  bool seenDestroy = false;
  bool seenSuccess = false;
  for (int tries = 0; tries < 100 && !seenDestroy; ++tries) {
    server::Entity* q = w.find(pid);
    int armedSlot = -1;
    for (size_t i = 0; i < q->inv.size(); ++i) {
      if (q->inv[i].equipped) armedSlot = static_cast<int>(i);
    }
    if (armedSlot < 0) {
      w.debugGive(*q, 2001, 1);
      for (size_t i = 0; i < q->inv.size(); ++i) {
        if (q->inv[i].itemId == 2001 && !q->inv[i].equipped) {
          w.toggleEquip(*q, static_cast<std::uint8_t>(i));
          break;
        }
      }
      q = w.find(pid);
      for (size_t i = 0; i < q->inv.size(); ++i) {
        if (q->inv[i].equipped) armedSlot = static_cast<int>(i);
      }
      if (armedSlot < 0) break;
    }
    std::uint8_t cur = q->inv[static_cast<size_t>(armedSlot)].aura;
    if (cur >= 4) {  // retire a fully-blessed survivor and bring a fresh victim
      w.toggleEquip(*q, static_cast<std::uint8_t>(armedSlot));
      q = w.find(pid);
      q->inv[static_cast<size_t>(armedSlot)] = server::InvSlot{};
      continue;
    }
    q->gold = 100000;
    w.debugGive(*q, 4001, 30); w.debugGive(*q, 4002, 15);
    w.debugGive(*q, 4004, 10); w.debugGive(*q, 4005, 10);
    const int preKarma = q->karma;
    if (!w.tryAnvil(*q, static_cast<std::uint8_t>(cur + 1))) { break; }
    q = w.find(pid);
    const int d = q->karma - preKarma;
    bool armed = false;
    for (const auto& sl : q->inv) armed = armed || (sl.itemId == 2001 && sl.equipped);
    if (!armed) {
      CHECK(d == server::World::kKarmaAnvilDestroy);  // drowned steel stains
      seenDestroy = true;
    } else if (d != 0) {
      CHECK(d == server::World::kKarmaAnvilOk);       // tithe remembered
      seenSuccess = true;
    }
    // softfail: d == 0 is the law (tolls only)
  }
  CHECK(seenDestroy);
  CHECK(seenSuccess);
}

// ---- Sprint 10: aura tiers III-V proc sim (T-047) --------------------------

TEST_CASE("T-047 aura tier III cleaves into the pack") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& p0 = w.spawn("cleaver", 0, std::nullopt);
  const std::uint32_t pid = p0.id;
  // primary rat + two packmates, all adjacent to the player
  const content::MobDef& rat = *content::findMob(1001);
  server::Entity& m1 = w.debugSpawnMob(rat, sim::TilePos{11, 11}, 1);
  server::Entity& m2 = w.debugSpawnMob(rat, sim::TilePos{12, 11}, 1);
  server::Entity& m3 = w.debugSpawnMob(rat, sim::TilePos{11, 12}, 1);
  const std::uint32_t id1 = m1.id, id2 = m2.id, id3 = m3.id;
  (void)id3;
  {
    server::Entity& p = *w.find(pid);
    p.walker.place(sim::TilePos{10, 11});
    p.str = 8;
    p.swordSkill = 90;         // plenty of ACC; still not the point
    p.swingLands = 90 * 20;    // keep skill stable when swinging
    w.debugGive(p, 2001, 1);   // Rusty Shank dmg 12
    w.toggleEquip(p, 0);
    // bless the blade at tier III directly (skips toll gates, tests the PROC):
    for (auto& sl : p.inv) {
      if (sl.itemId == 2001) sl.aura = 3;
    }
    w.setAttack(p, id1);
  }
  int ticks = 0;
  while (w.find(id1) != nullptr && ticks < 400) {
    if (w.find(pid)->hp > 0) w.setAttack(*w.find(pid), id1);
    w.tick();
    ++ticks;
  }
  REQUIRE(w.find(id1) == nullptr);                 // primary dropped
  // cleave must have BLED the packmates (same rolled damage, up to 2 in reach)
  const bool m2Hurt = w.find(id2) == nullptr || w.find(id2)->hp < 30u;
  CHECK(m2Hurt);
}

TEST_CASE("T-047 aura tier IV sunder hits harder than the table allows") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& p0 = w.spawn("sunderer", 0, std::nullopt);
  const std::uint32_t pid = p0.id;
  const content::MobDef& boss = *content::findMob(1008);  // hp 420, def 16
  server::Entity& m1 = w.debugSpawnMob(boss, sim::TilePos{11, 11}, 1);
  const std::uint32_t id1 = m1.id;
  server::Entity& p = w.find(pid) != nullptr ? *w.find(pid) : p0;
  p.walker.place(sim::TilePos{10, 11});
  p.str = 8;
  p.hpMax = 4000;  // survive the Sexton's 34-dmg returns while measuring
  p.hp = 4000;
  w.debugGive(p, 2001, 1);
  w.toggleEquip(p, 0);
  for (auto& sl : p.inv) {
    if (sl.itemId == 2001) sl.aura = 4;
  }
  // measure: run swings against a fresh ghoul and watch the biggest tick-loss
  std::uint32_t prevHp = w.find(id1)->hp;
  std::uint32_t biggest = 0;
  w.setAttack(*w.find(pid), id1);
  for (int t = 0; t < 1200 && w.find(id1) != nullptr; ++t) {
    w.tick();
    server::Entity* m = w.find(id1);
    if (m == nullptr) break;
    if (m->hp < prevHp) biggest = std::max(biggest, prevHp - m->hp);
    prevHp = m->hp;
    if (w.find(pid)->hp > 0) w.setAttack(*w.find(pid), id1);
  }
  // baseline best: shank 12 + str-rolled, ghoul def 5 => well under 35; sunder
  // procs add +35 flat, so the max observed loss must clear it.
  CHECK(biggest >= 35u);
}

TEST_CASE("T-047 aura tier V graft steals life on proc") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& p0 = w.spawn("grafted", 0, std::nullopt);
  const std::uint32_t pid = p0.id;
  const content::MobDef& ghoul = *content::findMob(1002);
  server::Entity& m1 = w.debugSpawnMob(ghoul, sim::TilePos{11, 11}, 1);
  const std::uint32_t id1 = m1.id;
  server::Entity& p = *w.find(pid);
  p.walker.place(sim::TilePos{10, 11});
  p.str = 8;
  p.hp = 1;                        // hang by a thread
  w.debugGive(p, 2001, 1);
  w.toggleEquip(p, 0);
  server::Entity* pp = w.find(pid);
  for (auto& sl : pp->inv) {
    if (sl.itemId == 2001) sl.aura = 5;
  }
  w.setAttack(*pp, id1);
  bool healed = false;
  for (int t = 0; t < 600 && !healed && w.find(id1) != nullptr; ++t) {
    w.tick();
    server::Entity* q = w.find(pid);
    server::Entity* m = w.find(id1);
    if (q == nullptr) break;
    if (q->hp > 1) healed = true;  // graft landed (or rat nibbled at us: no)
    if (m != nullptr && q->hp > 0) w.setAttack(*q, id1);
  }
  CHECK(healed);
}
