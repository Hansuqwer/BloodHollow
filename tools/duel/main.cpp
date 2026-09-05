// bh_duel — balancer v2 (T-034): gear-aware deterministic duel harness.
// Offline (no network): builds an in-memory arena World, spawns one mob from
// content/mobs.h and one player with an explicit loadout, sets the duel off,
// and measures TTK / hp-loss / win-rate across seeded reps.  Feeds the era
// curve table (devlog 0006 balance packs) without soak noise — the v1 feed
// (T-031 killStats EMA) could not separate loadouts; this can.
//
//   bh_duel --mob 1003 --level 5 --weapon 2002 --aura 1 --reps 9
//   bh_duel --table              # era sweep: all mobs x level x gear track
//   bh_duel --selftest           # determinism: same seed twice, bit-equal
#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "content/items.h"
#include "content/mobs.h"
#include "sim/combat.h"
#include "world.h"

namespace bh {

namespace {

sim::Map makeArena() {
  sim::Map m;
  m.w = 40;
  m.h = 40;
  m.tileW = 64;
  m.tileH = 32;
  m.ground.assign(40 * 40, 0);
  m.zone.assign(40 * 40, 0);
  m.blocked.assign(40 * 40, 0);  // no spawners: duel controls the cast
  return m;
}

struct Loadout {
  std::uint8_t level = 1;
  std::string build = "str";   // str|vit|dex|mix (stat points per level plan)
  std::uint32_t weapon = 0;    // 0 = fists
  std::uint32_t armor = 0;
  std::uint8_t aura = 0;       // applied to the weapon slot (0..5)
  bool skill = false;          // weave Power Swing off cooldown
  std::uint16_t vials = 0;     // Blood Vials; auto-sip at <=50% hp
};

struct DuelOut {
  bool win = false;
  sim::Tick ticks = 0;          // engage -> kill (or cap/loss)
  std::uint32_t hpEnd = 0, hpMax = 0;
  std::uint64_t endHash = 0;
};

// Build the duelist exactly once per rep so the world defaults (incl. rng)
// stay the single source of determinism.
DuelOut runDuel(const content::MobDef& mob, const Loadout& lo,
                std::uint64_t seed, sim::Tick cap) {
  server::World w;
  w.loadFrom(makeArena());
  w.rng() = sim::Rng(seed);

  server::Entity& p = w.spawn("duelist", 0, std::nullopt);
  const std::uint32_t pid = p.id;
  (void)p;

  // level + stat build through the REAL progression path
  std::uint32_t xp = 0;
  for (std::uint8_t l = 1; l < lo.level; ++l) xp += sim::xpNext(l);
  if (xp > 0) w.debugAwardXp(*w.find(pid), xp);
  int pts = w.find(pid)->statPoints;
  int order = 0;
  while (pts-- > 0) {
    std::uint8_t s = 0;  // str
    if (lo.build == "vit") s = 1;
    else if (lo.build == "dex") s = 2;
    else if (lo.build == "mix") s = static_cast<std::uint8_t>(order++ % 3);
    w.assignStat(*w.find(pid), s);
  }

  // gear through the real inventory/equip path
  if (lo.weapon != 0) {
    w.debugGive(*w.find(pid), lo.weapon, 1);
    server::Entity& pl = *w.find(pid);
    for (auto& inv : pl.inv) {
      if (inv.itemId == lo.weapon && !inv.equipped && inv.itemId != 0) {
        inv.aura = lo.aura;
      }
    }
    // equipped = first matching slot index
    for (size_t i = 0; i < pl.inv.size(); ++i) {
      if (pl.inv[i].itemId == lo.weapon) {
        w.toggleEquip(pl, static_cast<std::uint8_t>(i));
        break;
      }
    }
  }
  if (lo.armor != 0) {
    w.debugGive(*w.find(pid), lo.armor, 1);
    server::Entity& pl = *w.find(pid);
    for (size_t i = 0; i < pl.inv.size(); ++i) {
      if (pl.inv[i].itemId == lo.armor && !pl.inv[i].equipped) {
        w.toggleEquip(pl, static_cast<std::uint8_t>(i));
        break;
      }
    }
  }
  if (lo.vials > 0) w.debugGive(*w.find(pid), 3001, lo.vials);

  // the dance floor
  server::Entity& m = w.debugSpawnMob(mob, sim::TilePos{20, 20});
  const std::uint32_t mobId = m.id;
  (void)m;
  w.find(pid)->walker.place(sim::TilePos{21, 20});
  w.find(pid)->attackTarget = mobId;

  DuelOut out;
  sim::Tick t0 = w.tickCount();
  for (sim::Tick t = 0; t < cap; ++t) {
    server::Entity* pl = w.find(pid);
    if (pl == nullptr || pl->dead) {  // lost
      out.ticks = w.tickCount() - t0;
      out.hpEnd = 0;
      out.hpMax = 1;
      out.endHash = w.worldHash();
      return out;
    }
    server::Entity* mo = w.find(mobId);
    if (mo == nullptr) {  // won
      out.win = true;
      out.ticks = w.tickCount() - t0;
      out.hpEnd = pl->hp;
      out.hpMax = pl->hpMax;
      out.endHash = w.worldHash();
      return out;
    }
    if (lo.skill && pl->attackTarget != 0) w.trySkill(*pl, 1, mobId);
    if (lo.vials > 0 && pl->hp * 2 <= pl->hpMax) {
      for (size_t i = 0; i < pl->inv.size(); ++i) {
        if (pl->inv[i].itemId == 3001 && pl->inv[i].qty > 0) {
          w.useItem(*pl, static_cast<std::uint8_t>(i));
          break;
        }
      }
    }
    w.tick();
  }
  out.ticks = w.tickCount() - t0;  // capped (neither died)
  server::Entity* pl = w.find(pid);
  if (pl != nullptr) {
    out.hpEnd = pl->hp;
    out.hpMax = pl->hpMax;
  }
  out.endHash = w.worldHash();
  return out;
}

struct Agg {
  int reps = 0, wins = 0;
  std::vector<double> ttkS;
  std::vector<double> hpFrac;
};

Agg duelSeries(const content::MobDef& mob, const Loadout& lo, int reps,
               sim::Tick cap, std::uint64_t baseSeed) {
  Agg a;
  for (int r = 0; r < reps; ++r) {
    const DuelOut o = runDuel(mob, lo, baseSeed + 0x9E3779B9ULL * (r + 1), cap);
    ++a.reps;
    if (o.win) {
      ++a.wins;
      a.ttkS.push_back(o.ticks / 20.0);
      a.hpFrac.push_back(o.hpMax ? static_cast<double>(o.hpEnd) / o.hpMax : 0.0);
    } else {
      a.ttkS.push_back(o.ticks / 20.0);  // time-to-loss/cap, reported as TTK too
      a.hpFrac.push_back(0.0);
    }
  }
  return a;
}

double pct(std::vector<double> v, double q) {
  if (v.empty()) return 0.0;
  std::sort(v.begin(), v.end());
  const size_t i = std::min(v.size() - 1,
                            static_cast<size_t>(q * (v.size() - 1) + 0.5));
  return v[i];
}

void printRow(const content::MobDef& mob, const Loadout& lo, const Agg& a) {
  std::printf("%u,%-17s,%u,%u,%s,%u,%u,%u,%s,%u,%d/%d,%.1f,%.1f,%.2f\n",
              mob.mobId, mob.name, mob.level, lo.level,
              lo.weapon ? std::to_string(lo.weapon).c_str() : "fists",
              lo.aura, lo.armor, lo.skill ? 1u : 0u, lo.build.c_str(),
              lo.vials, a.wins, a.reps, pct(a.ttkS, 0.5), pct(a.ttkS, 0.9),
              pct(a.hpFrac, 0.5));
}

std::uint32_t argU32(const char* s) {
  return static_cast<std::uint32_t>(std::strtoul(s, nullptr, 10));
}

}  // namespace

int duelMain(int argc, char** argv) {
  Loadout lo;
  std::uint32_t mobId = 1001;
  int reps = 7;
  sim::Tick cap = 2400;  // 2 min
  std::uint64_t seed = 0xDEA1D21EULL;
  bool table = false, selftest = false;
  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    auto next = [&](const char* dflt) -> const char* {
      return (i + 1 < argc) ? argv[++i] : dflt;
    };
    if (std::strcmp(a, "--mob") == 0) mobId = argU32(next("1001"));
    else if (std::strcmp(a, "--level") == 0) lo.level = static_cast<std::uint8_t>(argU32(next("1")));
    else if (std::strcmp(a, "--build") == 0) lo.build = next("str");
    else if (std::strcmp(a, "--weapon") == 0) lo.weapon = argU32(next("0"));
    else if (std::strcmp(a, "--armor") == 0) lo.armor = argU32(next("0"));
    else if (std::strcmp(a, "--aura") == 0) lo.aura = static_cast<std::uint8_t>(argU32(next("0")));
    else if (std::strcmp(a, "--skill") == 0) lo.skill = std::strcmp(next("1"), "1") == 0;
    else if (std::strcmp(a, "--vials") == 0) lo.vials = static_cast<std::uint16_t>(argU32(next("0")));
    else if (std::strcmp(a, "--reps") == 0) reps = static_cast<int>(argU32(next("7")));
    else if (std::strcmp(a, "--cap") == 0) cap = static_cast<sim::Tick>(argU32(next("2400")));
    else if (std::strcmp(a, "--seed") == 0) seed = std::strtoull(next("0"), nullptr, 0);
    else if (std::strcmp(a, "--table") == 0) table = true;
    else if (std::strcmp(a, "--selftest") == 0) selftest = true;
    else {
      std::fprintf(stderr,
                   "bh_duel [--mob ID] [--level N] [--build str|vit|dex|mix] "
                   "[--weapon ID] [--armor ID] [--aura 0..5] [--skill 0|1] "
                   "[--vials N] [--reps N] [--cap TICKS] [--seed S] "
                   "[--table] [--selftest]\n");
      return 2;
    }
  }

  if (selftest) {
    const content::MobDef* mob = content::findMob(1003);
    Loadout st{5, "str", 2002, 2101, 1, true, 0};
    const DuelOut a = runDuel(*mob, st, 0xC0FFEE, 4000);
    const DuelOut b = runDuel(*mob, st, 0xC0FFEE, 4000);
    const bool ok = a.win == b.win && a.ticks == b.ticks &&
                    a.hpEnd == b.hpEnd && a.endHash == b.endHash;
    std::printf("[duel-selftest] win=%d ticks=%lld hpEnd=%u hash=%016llx -> %s\n",
                a.win ? 1 : 0, static_cast<long long>(a.ticks), a.hpEnd,
                static_cast<unsigned long long>(a.endHash),
                ok ? "DETERMINISTIC" : "NONDET");
    return ok ? 0 : 3;
  }

  if (table) {
    std::printf("mobId,name,mobL,playerL,weapon,aura,armor,skill,build,vials,"
                "wins/reps,ttk_med_s,ttk_p90_s,endhp_med_frac\n");
    // gear track: era-curve ladder (Soma aura ramp on the blade)
    const Loadout track[] = {
        {/*level*/ 0, "str", 0, 0, 0, false, 0},        // fists
        {0, "str", 2001, 0, 0, false, 0},               // Rusty Shank
        {0, "str", 2002, 0, 0, false, 0},               // Pit Blade
        {0, "str", 2002, 2101, 1, false, 0},            // +armor, aura I
        {0, "str", 2002, 2101, 1, true, 4},             // +skill weave + vials
        {0, "str", 2002, 2102, 4, true, 6},             // bone plate, aura IV
    };
    for (const content::MobDef& mob : content::kMobs) {
      for (int dl = -1; dl <= 2; dl += (dl == -1 ? 1 : 2)) {  // L-1, L, L+2
        const int pl = mob.level + dl;
        if (pl < 1 || pl > 12) continue;
        for (Loadout t : track) {
          t.level = static_cast<std::uint8_t>(pl);
          printRow(mob, t, duelSeries(mob, t, reps, cap, seed));
        }
      }
    }
    return 0;
  }

  const content::MobDef* mob = content::findMob(mobId);
  if (mob == nullptr) {
    std::fprintf(stderr, "bh_duel: unknown mob %u\n", mobId);
    return 2;
  }
  std::printf("mobId,name,mobL,playerL,weapon,aura,armor,skill,build,vials,"
              "wins/reps,ttk_med_s,ttk_p90_s,endhp_med_frac\n");
  printRow(*mob, lo, duelSeries(*mob, lo, reps, cap, seed));
  return 0;
}

}  // namespace bh

#if !defined(BH_NO_MAIN)
int main(int argc, char** argv) { return bh::duelMain(argc, argv); }
#endif
