// T-049 regression: live vs replay command-application parity.
// The replay flake root cause was two hand-mirrored switch statements
// (processCommand vs the --replay-world loop) drifting apart:
//   * kAttack lost e.path.clear() on dead/missing targets (observed flake:
//     256-unit Q10 sub-tile slip, transient 1-of-N hash mismatches)
//   * kSkill lost the a==0 -> attackTarget fallback
//   * kUseItem read q.a instead of channel
//   * kBuy/kTradeItem lost the qty>0 guard
// Both sinks now route through applyWorldCommand(); these tests pin the
// semantics so a future edit cannot silently fork the two paths again.
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
  sim::SpawnDef sp;
  sp.x = 5;
  sp.y = 5;
  sp.w = 4;
  sp.h = 4;
  sp.mobId = 1001;  // Marsh Rat, passive
  sp.maxAlive = 3;
  sp.respawnTicks = 20;
  m.spawners.push_back(sp);
  return m;
}

server::Entity* findMob(server::World& w) {
  for (auto& e : w.entities()) {
    if (e.kind == server::EntityKind::kMob && e.wireKind < 64) return &e;
  }
  return nullptr;
}

server::Command cmd(server::Command::Kind k, std::int32_t a = 0,
                    std::int32_t b = 0, std::uint8_t channel = 0) {
  server::Command c;
  c.kind = k;
  c.a = a;
  c.b = b;
  c.channel = channel;
  return c;
}
}  // namespace

TEST_CASE("cmd parity (T-049): attack on dead/missing target cancels pathing") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& p = w.spawn("tester", 0, std::nullopt);
  const std::uint32_t pid = p.id;
  (void)p;
  server::Entity& pl = *w.find(pid);
  pl.walker.place(sim::TilePos{20, 20});
  w.queuePath(pl, sim::TilePos{30, 20});
  REQUIRE_FALSE(pl.path.empty());

  // Attack an entity id that does not exist (e.g. already-dead mob): the
  // intent still cancels the stale walk order, matching the live handler that
  // cleared the path before setAttack()'s early return.
  server::applyWorldCommand(w, pl, cmd(server::Command::kAttack, 0xDEAD));
  CHECK(pl.path.empty());
  CHECK(pl.attackTarget == 0);  // setAttack must still reject the bad target
}

TEST_CASE("cmd parity (T-049): attack then walk burst in one tick keeps only the last order") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& p = w.spawn("tester", 0, std::nullopt);
  const std::uint32_t pid = p.id;
  (void)p;

  // Burst (same tick, FIFO order): path -> attack(dead) -> path.
  // Live fair-share applies one per tick from a queue; replay re-applies the
  // journaled order — both must converge to the same permanent state.
  server::World w2;
  REQUIRE(w2.loadFrom(makeArena()));
  server::Entity& p2 = w2.spawn("tester", 0, std::nullopt);
  const std::uint32_t pid2 = p2.id;
  (void)p2;

  const server::Command burst[] = {
      cmd(server::Command::kPath, 30, 30),
      cmd(server::Command::kAttack, 0xDEAD),  // dead target: cancels path
      cmd(server::Command::kPath, 25, 25),
      cmd(server::Command::kSkill, 0, 0, 1),  // no target -> attackTarget(0) -> no-op
  };
  // live-style: apply one per tick
  for (const auto& c : burst) {
    server::Entity& e = *w.find(pid);
    server::applyWorldCommand(w, e, c);
    w.tick();
  }
  // replay-style: all before the ticks, then same number of ticks
  for (const auto& c : burst) {
    server::Entity& e = *w2.find(pid2);
    server::applyWorldCommand(w2, e, c);
  }
  for (int i = 0; i < 4; ++i) w2.tick();

  const server::Entity& e1 = *w.find(pid);
  const server::Entity& e2 = *w2.find(pid2);
  CHECK(e1.walker.x == e2.walker.x);
  CHECK(e1.walker.y == e2.walker.y);
  CHECK(e1.hp == e2.hp);
  CHECK(e1.path.size() == e2.path.size());  // last order wins in both
  CHECK(e1.attackTarget == e2.attackTarget);
}

TEST_CASE("cmd parity (T-049): skill with a==0 falls back to attackTarget") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* mob = findMob(w);
  REQUIRE(mob != nullptr);
  const std::uint32_t mobId = mob->id;

  server::Entity& p = w.spawn("tester", 0, std::nullopt);
  const std::uint32_t pid = p.id;
  (void)p;
  server::Entity& pl = *w.find(pid);
  pl.walker.place(sim::TilePos{mob->walker.tile().x + 1, mob->walker.tile().y});
  w.setAttack(pl, mobId);
  REQUIRE(pl.attackTarget == mobId);

  w.tick();  // tick_ = 1 so lastPowerTick (-1000) differs observably
  // a==0 must resolve to attackTarget; if a regressed to passing 0 verbatim,
  // trySkill would find nothing and never touch lastPowerTick.
  server::applyWorldCommand(w, *w.find(pid), cmd(server::Command::kSkill, 0, 0, 1));
  CHECK(w.find(pid)->lastPowerTick == w.tickCount());
}

TEST_CASE("cmd parity (T-049): useItem consumes the channel slot, not field a") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& p = w.spawn("tester", 0, std::nullopt);
  const std::uint32_t pid = p.id;
  (void)p;
  server::Entity& pl = *w.find(pid);
  pl.hp = pl.hpMax > 100 ? pl.hpMax - 100 : 1;  // wounded so the vial can heal
  server::InvSlot vial;
  vial.itemId = 3001;  // Blood Vial, consumable
  vial.qty = 2;
  pl.inv.push_back(vial);

  // Old replay handled kUseItem with q.a; live uses channel.  Send junk in a.
  server::applyWorldCommand(w, pl, cmd(server::Command::kUseItem, 9, 0, 0));
  const server::Entity& after = *w.find(pid);
  REQUIRE(after.inv.size() == 1);
  CHECK(after.inv[0].qty == 1);  // one vial sipped from slot 0
  CHECK(after.hp > (after.hpMax > 100 ? after.hpMax - 100 : 1));
}

TEST_CASE("cmd parity (T-049): buy with qty<=0 is a no-op") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity& p = w.spawn("tester", 0, std::nullopt);
  const std::uint32_t pid = p.id;
  (void)p;
  const std::uint32_t goldBefore = w.find(pid)->gold;
  server::applyWorldCommand(w, *w.find(pid),
                            cmd(server::Command::kBuy, 2001, 0, 0));
  CHECK(w.find(pid)->gold == goldBefore);
  CHECK(w.find(pid)->inv.empty());
}
