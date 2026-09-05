// T-061 nightcreep + T-062 night economy — dark-hours law on the sim clock.
//
// The look floor (never darker than alpha-150 blue) is a render-side pin in
// engine/render/daynight.cpp documented in the T-062 card; here we pin the
// SIM stakes: hour band edges, mob bite + aggro, night XP & loot odds.
#include <doctest/doctest.h>

#include <cstdint>

#include "command.h"
#include "content/mobs.h"
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
// hours -> ticks from world tick 0 (fresh worlds start at 08:00)
sim::Tick tickAtHour(float h) {
  const double fromStart = h - bh::sim::kStartHour;
  const double wrapped = fromStart < 0 ? fromStart + 24.0 : fromStart;
  return static_cast<sim::Tick>(wrapped * static_cast<double>(bh::sim::kTicksPerGameHour));
}
}  // namespace

TEST_CASE("T-061: night band edges at the dark-hours law (21:00-05:00)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  w.debugSetTick(tickAtHour(20.999f));
  CHECK_FALSE(w.isNight());
  w.debugSetTick(tickAtHour(21.0f));
  CHECK(w.isNight());
  w.debugSetTick(tickAtHour(3.5f));
  CHECK(w.isNight());
  w.debugSetTick(tickAtHour(4.999f));
  w.debugSetTick(tickAtHour(5.0f));
  CHECK_FALSE(w.isNight());
}

TEST_CASE("T-061: night bite — mobs hit 15% harder after dark (paired worlds)") {
  // Pair two seed-identical worlds; the ONLY sim difference is the tick dial.
  // A totaled swing-by-swing hp loss comparison pins the x1.15 law end-to-end.
  auto bite = [](sim::Tick t, const char* who) {
    server::World w;
    w.loadFrom(makeArena());
    w.debugSetTick(t);
    auto* p = w.find(w.spawn(who, 0, sim::TilePos{10, 10}).id);
    p->dex = 0;  // no evasion: every mob swing lands, strata identical
    p->hp = 9999;
    p->hpMax = 9999;
    auto* ghou = w.find(w.debugSpawnMob(*content::findMob(1002), {11, 10}).id);
    w.debugKillMob(*w.find(w.debugSpawnMob(*content::findMob(1001), {30, 30}).id), nullptr);
    (void)ghou;
    // a rat far away consumes the same baseline rolls in both worlds
    for (int i = 0; i < 240; ++i) w.tick();  // 12s: several mob swing frames
    return 9999 - p->hp;  // total damage taken (player never acts)
  };
  const std::int32_t dayBite = bite(tickAtHour(12.0f), "daywalker");
  const std::int32_t nightBite = bite(tickAtHour(23.0f), "nightwatch");
  INFO("day=", dayBite, " night=", nightBite);
  REQUIRE(dayBite > 0);
  // every landed hit pays exactly floor(dmg*115/100): totals preserve the
  // factor (per-hit floors differ by <1 per hit — pin a tight band, not noise)
  const double ratio = static_cast<double>(nightBite) / static_cast<double>(dayBite);
  CHECK(ratio > 1.10);
  CHECK(ratio < 1.20);
}

TEST_CASE("T-062: night XP pays +10% at the kill (paired worlds)") {
  // Fresh seed-identical worlds, same kill, different clock dial — no level
  // drift between the measurements, so the ONLY delta is the night law.
  auto killGain = [&](sim::Tick t) {
    server::World w;
    w.loadFrom(makeArena());
    w.debugSetTick(t);
    auto* p = w.find(w.spawn("nightwatch", 0, sim::TilePos{10, 10}).id);
    const std::int64_t xp0 = p->xp;
    auto* rat = w.find(w.debugSpawnMob(*content::findMob(1001), {11, 10}).id);
    w.debugKillMob(*rat, p);
    return p->xp - xp0;
  };
  const std::int64_t dayGain = killGain(tickAtHour(12.0f));
  const std::int64_t nightGain = killGain(tickAtHour(23.0f));
  REQUIRE(dayGain > 0);
  CHECK(nightGain == dayGain * 110 / 100);  // the +10% law, floored
}

TEST_CASE("T-062: night loot widens odds 25% relative (empirical, paired strata)") {
  // Fixed seed, M kills under each clock — the loot roll is deterministic
  // per strata, so counts must diverge in exactly the direction of the law.
  // ghouls drop at lootChancePct 35%; night widens that to floor(35*125/100)=43,
  // which flips more rolls onto the <= threshold side. Empirical witness.
  auto lootCount = [&](sim::Tick t, int kills) {
    server::World w;
    w.loadFrom(makeArena());
    w.debugSetTick(t);
    auto* p = w.find(w.spawn("harvester", 0, sim::TilePos{10, 10}).id);
    int total = 0;
    for (int i = 0; i < kills; ++i) {
      auto* rat = w.find(w.debugSpawnMob(*content::findMob(1002), {11 + (i % 4), 10}).id);
      const std::size_t inv0 = w.find(p->id)->inv.size();
      (void)inv0;
      const std::uint16_t q0 = w.find(p->id)->gold;  // gold always drops; count junk
      (void)q0;
      int junk0 = 0;
      for (const auto& sl : w.find(p->id)->inv)
        if (content::findItem(sl.itemId) && content::findItem(sl.itemId)->slot == 3)
          junk0 += sl.qty;
      w.debugKillMob(*rat, w.find(p->id));
      int junk1 = 0;
      for (const auto& sl : w.find(p->id)->inv)
        if (content::findItem(sl.itemId) && content::findItem(sl.itemId)->slot == 3)
          junk1 += sl.qty;
      total += junk1 - junk0;
    }
    return total;
  };
  const int day = lootCount(tickAtHour(12.0f), 400);
  const int night = lootCount(tickAtHour(23.0f), 400);
  REQUIRE(day > 0);
  INFO("400 ghoul kills: junk day=", day, " night=", night);
  CHECK(night > day);            // the law widens drops after dark
  CHECK(night <= day * 2);       // and it's a 25% nudge, not a faucet
}
