// T-050 party core + T-051 XP share, pinned at World-API level.
// Semantics locked in docs/tasks: session-scoped (no persistence), leader-only
// invite/kick, invites die after 200 ticks, leader-leave -> eldest inherits,
// empty party disbands, share radius 12 tiles Chebyshev around the KILL,
// bonus +12% XP per extra in-range sharer, loot/gold stay with the killer.
// Journal invariant (tested live-vs-replay style): ops applied as id-resolved
// Commands replay to identical party state.
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
#include "content/mobs.h"
#include "sim/combat.h"
#include "world.h"

using namespace bh;

namespace {
sim::Map makeArena() {
  sim::Map m;
  m.w = 60;
  m.h = 60;
  m.tileW = 64;
  m.tileH = 32;
  m.ground.assign(60 * 60, 0);
  m.zone.assign(60 * 60, 0);
  m.blocked.assign(60 * 60, 0);
  return m;  // no spawners: tests kill an explicitly spawned mob
}

server::Entity* spawnP(server::World& w, const char* name, int x, int y) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y});
  return w.find(e.id);
}

server::Entity* spawnRat(server::World& w, int x, int y) {
  const content::MobDef* md = content::findMob(1001);  // Marsh Rat, xp 40
  REQUIRE(md != nullptr);
  server::Entity& m = w.debugSpawnMob(*md, sim::TilePos{x, y});
  return w.find(m.id);
}

server::Command cmd(server::Command::Kind k, std::int32_t a = 0) {
  server::Command c;
  c.kind = k;
  c.a = a;
  return c;
}
}  // namespace

TEST_CASE("T-050: invite bootstraps a party; range/dead/self/non-leader rejected; cap at 8") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* a = spawnP(w, "alpha", 10, 10);
  auto* b = spawnP(w, "beta", 11, 10);
  auto* far = spawnP(w, "gamma", 50, 10);  // Chebyshev 40 > 12

  CHECK_FALSE(w.partyInvite(*a, *a));   // self
  CHECK(w.partyInvite(*a, *b));         // creates party, a is leader
  REQUIRE(a->partyId != 0);
  CHECK(w.partyOf(a->id)->leaderId == a->id);
  CHECK(w.partyOf(a->id)->members.size() == 1);

  CHECK_FALSE(w.partyInvite(*a, *far));  // out of 12-tile radius
  far->dead = true;
  CHECK_FALSE(w.partyInvite(*a, *far));  // dead rejected
  far->dead = false;

  REQUIRE(w.partyAccept(*b));            // beta joins: 2 members

  // non-leader member cannot invite
  auto* t2 = spawnP(w, "fourth", 13, 10);
  CHECK_FALSE(w.partyInvite(*b, *t2));

  // dead target rejected
  auto* t3 = spawnP(w, "fifth", 12, 11);
  t3->dead = true;
  CHECK_FALSE(w.partyInvite(*a, *t3));
  t3->dead = false;

  // fill to the cap of 8 (a + b + 6)
  int added = static_cast<int>(w.partyOf(a->id)->members.size());
  for (int i = added; i < server::World::kPartyMaxMembers; ++i) {
    std::string n = "fill" + std::to_string(i);
    auto* m = spawnP(w, n.c_str(), 10 + (i % 5), 11 + (i % 3));
    REQUIRE(w.partyInvite(*a, *m));
    REQUIRE(w.partyAccept(*m));
  }
  CHECK(w.partyOf(a->id)->members.size() == server::World::kPartyMaxMembers);
  auto* ninth = spawnP(w, "nine", 10, 12);
  CHECK_FALSE(w.partyInvite(*a, *ninth));  // capped
}

TEST_CASE("T-050: leader leave -> eldest inherits; kick; empty disbands") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* a = spawnP(w, "alpha", 10, 10);
  auto* b = spawnP(w, "beta", 11, 10);
  auto* c = spawnP(w, "cara", 12, 10);
  const std::uint32_t aid = a->id, bid = b->id, cid = c->id;

  REQUIRE(w.partyInvite(*a, *b));
  REQUIRE(w.partyAccept(*w.find(bid)));
  REQUIRE(w.partyInvite(*a, *c));
  REQUIRE(w.partyAccept(*w.find(cid)));
  CHECK(w.partyOf(aid)->members.size() == 3);

  // leader leaves -> eldest remaining (beta, joined before cara) inherits
  REQUIRE(w.partyLeave(*w.find(aid)));
  CHECK(w.find(aid)->partyId == 0);
  const auto* p = w.partyOf(bid);
  REQUIRE(p != nullptr);
  CHECK(p->leaderId == bid);
  CHECK(p->members.size() == 2);

  // non-leader kick rejected; leader kick works
  CHECK_FALSE(w.partyKick(*w.find(cid), bid));
  CHECK(w.partyKick(*w.find(bid), cid));
  CHECK(w.find(cid)->partyId == 0);
  CHECK(w.partyOf(bid)->members.size() == 1);

  // last member out -> disband
  REQUIRE(w.partyLeave(*w.find(bid)));
  CHECK(w.partyOf(bid) == nullptr);
  CHECK(w.parties().empty());
}

TEST_CASE("T-050: invite expires after 200 ticks") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* a = spawnP(w, "alpha", 10, 10);
  auto* b = spawnP(w, "beta", 11, 10);
  REQUIRE(w.partyInvite(*a, *b));
  for (int i = 0; i < 201; ++i) w.tick();
  CHECK_FALSE(w.partyAccept(*b));
}

TEST_CASE("T-050: despawn sweeps membership (leader handoff by join order)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* a = spawnP(w, "alpha", 10, 10);
  auto* b = spawnP(w, "beta", 11, 10);
  const std::uint32_t aid = a->id, bid = b->id;
  REQUIRE(w.partyInvite(*a, *b));
  REQUIRE(w.partyAccept(*w.find(bid)));

  w.despawn(aid);  // leader disconnected -> beta leads
  const auto* p = w.partyOf(bid);
  REQUIRE(p != nullptr);
  CHECK(p->leaderId == bid);
  CHECK(p->members.size() == 1);

  w.despawn(bid);  // empty -> disband
  CHECK(w.parties().empty());
}

TEST_CASE("T-051: XP splits around the kill; far/dead/zone gates; solo unchanged") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  auto* killer = spawnP(w, "killer", 10, 10);
  auto* mate = spawnP(w, "mate", 11, 10);    // in range
  auto* farer = spawnP(w, "farer", 30, 10);  // 20 tiles out
  const std::uint32_t kid = killer->id, mid = mate->id, fid = farer->id;
  REQUIRE(w.partyInvite(*killer, *mate));
  REQUIRE(w.partyAccept(*w.find(mid)));

  auto* mob = spawnRat(w, 10, 10);
  REQUIRE(mob != nullptr);
  w.debugKillMob(*mob, w.find(kid));

  // 2 sharers: each = 40 * (100 + 12) / 100 / 2 = 4480/200 = 22
  const std::uint32_t expect = 40u * 112u / 100u / 2u;
  CHECK(expect == 22);
  CHECK(w.find(kid)->xp == expect);   // < xpNext(1) = 100: no level-up wrap
  CHECK(w.find(mid)->xp == expect);
  CHECK(w.find(fid)->xp == 0);        // not in party at all

  // dead member doesn't share: kill another rat with mate dead
  w.find(mid)->dead = true;
  auto* mob2 = spawnRat(w, 10, 10);
  w.debugKillMob(*mob2, w.find(kid));
  CHECK(w.find(kid)->xp == expect + 40u);  // solo payout now
  CHECK(w.find(mid)->xp == expect);

  // solo player baseline: full pool to the killer
  server::World w2;
  REQUIRE(w2.loadFrom(makeArena()));
  auto* solo = spawnP(w2, "solo", 10, 10);
  auto* mob3 = spawnRat(w2, 10, 10);
  const std::uint32_t sid = solo->id;
  w2.debugKillMob(*mob3, w2.find(sid));
  CHECK(w2.find(sid)->xp == 40u);
}

TEST_CASE("T-050/51: party ops as id-resolved Commands replay identically") {
  // live: interleave commands + ticks (one fair-share step at a time)
  server::World livew;
  REQUIRE(livew.loadFrom(makeArena()));
  const std::uint32_t lai = spawnP(livew, "alpha", 10, 10)->id;
  const std::uint32_t lbi = spawnP(livew, "beta", 11, 10)->id;
  {
    server::applyWorldCommand(livew, *livew.find(lai),
                              cmd(server::Command::kPartyInvite, lbi));
    livew.tick();
    server::applyWorldCommand(livew, *livew.find(lbi),
                              cmd(server::Command::kPartyAccept));
    livew.tick();
  }
  auto* lm = spawnRat(livew, 10, 10);
  livew.debugKillMob(*lm, livew.find(lai));

  // replay: journaled commands re-applied up front, same tick count
  server::World replayw;
  REQUIRE(replayw.loadFrom(makeArena()));
  const std::uint32_t rai = spawnP(replayw, "alpha", 10, 10)->id;
  const std::uint32_t rbi = spawnP(replayw, "beta", 11, 10)->id;
  {
    server::applyWorldCommand(replayw, *replayw.find(rai),
                              cmd(server::Command::kPartyInvite, rbi));
    server::applyWorldCommand(replayw, *replayw.find(rbi),
                              cmd(server::Command::kPartyAccept));
    replayw.tick();
    replayw.tick();
  }
  auto* rm = spawnRat(replayw, 10, 10);
  replayw.debugKillMob(*rm, replayw.find(rai));

  const auto* lp = livew.partyOf(lai);
  const auto* rp = replayw.partyOf(rai);
  REQUIRE(lp != nullptr);
  REQUIRE(rp != nullptr);
  CHECK(lp->members == rp->members);
  CHECK(lp->leaderId == rp->leaderId);
  CHECK(livew.find(lai)->xp == replayw.find(rai)->xp);
  CHECK(livew.find(lbi)->xp == replayw.find(rbi)->xp);
}
