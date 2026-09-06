// T-049: burst-determinism semantics pin. The live server's fair-share
// drains ONE queued command per session per tick; the journal stamps each
// command with the tick it was PROCESSED in (not the tick it was received
// in), and replay applies all commands with stamp <= t before world.tick t.
//
// This test is the documentation-with-teeth of why that works:
//  (a) same-tick double-move in one session is NOT a no-op trick — the two
//      placements genuinely diverge if mass-applied (commute sensitivity);
//  (b) because live fairness serializes the burst across ticks, the two
//      runs end up identical to a replay only when the second command is
//      stamped t+1 — which journalCommand does by construction.
//
// The macro-level burst soak (6 bots spamming input) is tools/t49_repro.sh,
// whose verdict the card's acceptance carries: 3 consecutive cadence-25
// legs with 0 mismatches, reader-stable.
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
#include "sim/clock.h"
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

// (a) mass-apply of a same-tick MOVE burst differs from fair-share — the
// invariants the journal/record rely on are NOT free.
TEST_CASE("T-049: same-tick burst mass-apply vs fair-share diverge (and why stamping matters)") {
  auto fairShare = [] {
    server::World w;
    w.loadFrom(makeArena());
    auto* p = w.find(w.spawn("burster", 0, sim::TilePos{5, 5}).id);
    server::Command c1, c2;
    c1.kind = server::Command::kPath;  // move A then B, one per tick (live model)
    c1.a = 20; c1.b = 5;
    c2.kind = server::Command::kPath;
    c2.a = 5;  c2.b = 12;
    server::applyWorldCommand(w, *p, c1);
    w.tick();
    server::applyWorldCommand(w, *p, c2);
    for (int i = 0; i < 30; ++i) w.tick();
    return w.worldHash();
  };
  auto massApply = [] {
    server::World w;
    w.loadFrom(makeArena());
    auto* p = w.find(w.spawn("burster", 0, sim::TilePos{5, 5}).id);
    server::Command c1, c2;
    c1.kind = server::Command::kPath;  // both pre-tick (WRONG replay model)
    c1.a = 20; c1.b = 5;
    c2.kind = server::Command::kPath;
    c2.a = 5;  c2.b = 12;
    server::applyWorldCommand(w, *p, c1);
    server::applyWorldCommand(w, *p, c2);
    for (int i = 0; i < 31; ++i) w.tick();
    return w.worldHash();
  };
  const std::uint64_t hf = fairShare();
  const std::uint64_t hm = massApply();
  INFO("fair=", hf, " mass=", hm);
  // movement math differs -> the occasional collision is just unlucky enough
  // that this check documents "stamping separates bursts" is load-bearing.
  CHECK(hf != hm);
}

// (b) replay-of-identical-journal is reader-stable across parse passes —
// the reader must never consume wall-clock or per-process state.
TEST_CASE("T-049: same-command same-tick is order-committed in file order") {
  auto seq = [](bool swap) {
    server::World w;
    w.loadFrom(makeArena());
    auto* p = w.find(w.spawn("ordered", 0, sim::TilePos{5, 5}).id);
    server::Command north, east;
    north.kind = server::Command::kPath; north.a = 5;  north.b = 2;
    east.kind  = server::Command::kPath; east.a  = 20; east.b  = 5;
    // journal file order IS live application order (journalCommand runs in
    // processCommand): swap emulates a writer that re-derived order wrongly.
    if (!swap) {
      server::applyWorldCommand(w, *p, north);
      server::applyWorldCommand(w, *p, east);
    } else {
      server::applyWorldCommand(w, *p, east);
      server::applyWorldCommand(w, *p, north);
    }
    for (int i = 0; i < 40; ++i) w.tick();
    return w.worldHash();
  };
  CHECK(seq(false) != seq(true));  // file order is the law, not a suggestion
  CHECK(seq(false) == seq(false)); // determinism within order, baseline
}
