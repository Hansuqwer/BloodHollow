# Session handoff — campaign bot progression (T-034 series → Route v5c)

*Written 2026-09-07 for the next agent session. Read `AGENTS.md` first; this
brief assumes it.*

## Mission

BLOODHOLLOW (2D MMORPG, C++20 raylib client + headless server, ENet, SQLite).
This workstream makes the **campaign bots** (`tools/bots/main.cpp`, profile
`campaign`) climb the level ladder on map 1 (Thornwall) as a live balance
probe: bot progression is the fidelity proxy for player progression. The
director's target is **L8 reachable end-to-end** (`TARGET L8` in the chain
gate).

## Where things stand (HEAD `af06038`, pushed to `origin/master`)

Working tree clean except `docs/prompts/arena-parallel-eval-brief.md`
(untracked) — **that file belongs to a parallel workstream, do not touch,
move, or commit it.**

A second workstream (art enablement: T-ART-01..11, era-violet palette,
atlaspack tooling) commits to `master` concurrently (`44ad327`, `56aca48`,
`78aa6c9`). **Always `git pull --rebase` before push** and expect your push to
be a fast-forward race.

### What was done, in order

| stage | evidence | outcome |
|---|---|---|
| T-034a-c: death debt, rerun plateau, hound economics | devlogs 0025-0026 | baseline parked at L4-L5 |
| T-034d: re-gear trip (walk home when gold clears next gear tier) | devlog 0027, `732a40c` | plateau moved to L6 in 4/12 legs |
| 0028 chain rerun | devlog 0028 | L6 wall identified — hypothesis blamed "over-level gnoll/widow triangle" (**later disproved**) |
| death diagnostics | devlog 0029, `a262909` | `killerByLvl` + `lastDeath` SUMMARY counters in `tools/bots` |
| Route v5c | devlog 0029, `a2dcc6b` | the wall was **pack density at the own waypoint**, not over-level mobs |
| v5c 12-leg chain | `af06038` | floor up a full level; ceiling still L6 |

### The diagnosis that matters (do not re-derive it)

Devlog 0028's "over-level triangle" hypothesis was **wrong**. Diagnostics
showed the killers were L3 Feral Ghouls, and `lastDeath` clustered at
`(55,18)/(55,20)` — the centre of the `ghouls_east` spawn rect
(`tools/mapgen/make_thornwall.py` ~line 156: x[52,58] y[17,22], **maxAlive
8**), which Route v4 parked every L3+ bot on. Secondary bug: the ±10
threat-axis retreat walked bots 3 tiles past the **L11 Gravecaller barricade**
into `hounds_marsh`.

### Route v5c (current bot logic, `tools/bots/main.cpp` only)

1. **Edge-stand the ladder waypoint**: `(55,19)→(55,14)` instead of standing
   inside the spawn rect.
2. **Level-blind pack cap** for L3+ (skip targets when visible count ≥ cap;
   passive rats excluded via `entAggressive()`). *Gotcha: a cap of 3 zeroes XP
   gain entirely — measured, not assumed.*
3. **Retreat home** on map 1 (not threat-axis), sticky until healed **and**
   unpursued.
4. **Defend while fleeing** — attack at point-blank inside the retreat branch.
   *Gotcha: an early draft `continue`d before the attack block; fleeing bots
   never swung.*
5. **Swarm panic-break at 5**.

### Measured results (12-leg chain, fresh DB, 2 bots, ~10.8k ticks/leg)

All 12 journals replay **0 mismatches**; `TARGET L8` never fires. vs 0028
baseline: mean end level 4.08 → **4.58**; legs ending L5+ 5/12 → **8/12**;
worst close L2 (was L1); deaths 740 → 662. Full table in devlog 0029 and the
board (`docs/tasks/README.md`, "Done — Route v5c"). val4 (resumed DB at L5)
reached **L7 with 0 deaths on bot 00** — but that used banked gear; a fresh
chain still closes legs at L5-L6.

## The one open card: L8 content gate

L7→L8 has **no reachable step-up on map 1**: `gnolls_pits` (L7) and
`widow_glade` (L9) sit south of the river, and the only bridge passes the
**L11 gravecaller barricade at `(6,18)`**. This is a **content decision** —
move the barricade, add a crossing, or re-anchor the gnolls — for the
director to choose. Bot tuning is exhausted; do not burn sessions trying to
tune past it. If the director picks an option, the change belongs in
`tools/mapgen/make_thornwall.py` (+ regenerate the map) and needs its own
card + devlog.

## How to run things (hard-won environment facts)

- **Per-command timeout ~30 s.** Long runs must go detached:
  `setsid bash -c 'exec <script> > logs/<run>.log 2>&1' &` then poll with
  short greps. A full 12-leg chain takes **~1 h 50 min**; leg cadence is
  ~9 min (look for `FINAL_RERUN_LEG<N>_DONE` / `leg<N> verified` in the gate
  log). A waiter-script + marker-file pattern (`/tmp/chain_waiter.sh`,
  `/tmp/chain_done.marker`) was used last session — recreate as needed.
- **Chain harness**: `tools/m2b_rerun_chain.sh` (12 legs, fresh DB at
  `/tmp/m2b_rerun_campaign.db`, chars persist across legs). Single leg:
  `tools/m2b_leg.sh`. Gate log: `logs/m2b_rerun_gate.log`; per-leg
  `logs/m2b_rerun_leg<N>_{bots,server}.log` + `.bwj` journals (`.bwj` files
  ARE tracked in git, pre-dating the `.gitignore` rule — commit them as
  evidence, like previous sessions did).
- **Resumed-DB validation runs** (val1..val4 pattern): copy the chain DB,
  `sqlite3` to set character level/gear, run `bh_server` + `bh_bots` for
  540 s, grep `reached L` + `SUMMARY`. Same seed must reproduce val2/val4.
- **`maxLevel` semantics**: it is the level at leg close (SUMMARY counter),
  NOT the peak. Peak = last `reached L<N>` line. Keep this distinction in any
  table you write (devlogs 0028/0029 use `peak` / `end`).
- **Verification protocol before any commit**: `cmake --build build -j"$(nproc)"`
  warning-free, `ctest --test-dir build` (2/2), `./build/tests/bh_tests`
  (**105 tests / 327,925 assertions**), and **replay 0 mismatches** for every
  leg touched. Journal epoch is currently **6** — bot-only changes must not
  bump it.
- **Git identity is unset** on this machine; previous commits used
  `git -c user.name=... -c user.email=...` (see `git log`). Match the
  existing author convention or ask the director which identity to use.
- **Push target**: `origin/master` at
  `https://github.com/Hansuqwer/BloodHollow` (gh auth is authenticated).

## Suggested next steps

1. `git pull --rebase`, build, run the test suite — confirm the baseline is
   green before anything else.
2. Take the **L8 content gate** question to the director with the three
   options above (barricade move / new crossing / gnoll re-anchor) and the
   trade-offs from devlog 0029's "Scope boundary that remains" section.
3. If the director wants more bot-tuning work instead, candidate threads with
   existing evidence: deaths still ~10% heavy on bad legs (142/134/116) — the
   pack cap and retreat threshold are the levers; leg 3 and leg 5 logs are the
   reproductions.
4. Whatever runs you start: detach them, keep the gate-log discipline, and
   paste leg tables into a new devlog (`0030-...`) — one devlog per
   hypothesis, like 0025→0029.

