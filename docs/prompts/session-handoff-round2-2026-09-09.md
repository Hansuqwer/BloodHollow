# Session handoff — round-2 run (2026-09-09, director-ordered)

*Follows `docs/prompts/session-handoff-1to10-2026-09-09.md` (HEAD `7f512c5`).
Director ordered the round-2 todolist (`next-todolist-10-tasks-round2-2026-09-09.md`)
executed 1–10 in order. All 10 landed as one card = one sprint = one devlog =
one commit, all pushed to `origin/master`.*

## Ground state (verified after the run)

- HEAD `4b4ab47` — "T-099: trade-pass human review packet (devlog 0068)",
  pushed to `origin/master`.
- Suite: **173/173 tests, 328,624 assertions** (was 167/167, 328,566),
  `ctest --test-dir build` 2/2, build warning-free (`-Wall -Wextra -Werror`).
- Journal epoch **14** (12→13 T-091 slam, 13→14 T-094 NPC posts + anvil fix).
  Wire `kProtocolVersion` **237** (untouched — trailing appends per count law).
  `.bhmap` **v2**.
- Latest replay proof: `./build/server/bh_server --replay-world logs/t094.bwj`
  → `[replay] OK ticks=12801 sessionCmds=7240 hashes=513 mismatches=0 entities=180`.
- Live entity count **~360 → ~180** (one-anvil fix); p99 **~1.3–1.6 ms** (was 3.8–5.9).
- Working tree (leave alone — another workstream's):
  `M tools/atlaspack/bh_mob_sheet.py`,
  `?? docs/prompts/arena-parallel-eval-brief.md`,
  `?? docs/prompts/art-b1-b8-execution.md`,
  `?? tools/atlaspack/b5_build.sh`. Never stage these.
  (Also untracked, mine: `next-todolist-10-tasks-*.md` scratch prompts +
  `logs/t*_runner.log` runner scraps — delete or ignore at will.)
- Git identity for this workstream:
  `git -c user.name=bloodhollow-dev -c user.email=dev@bloodhollow.local`.

## What the run did (all in `docs/tasks/done/` + devlogs 0059–0068)

| card | content | evidence |
|---|---|---|
| T-090 | L8→L9 climb: 540 s from 0 xp, 156 kills / 2 roam deaths, 76%+57% bar — ladder OPEN, pace-limited, no wall | t090.bwj OK |
| T-091 | Gravemother 60t wind-up + radius-2 slam replaces instant bolt; epoch 12→13 | t091.bwj OK, t084 refuses exit 4 |
| T-092 | trailing `glowTier` on spawn/delta, scan + halo; wire stays 237, epoch stays 13 | t091 replays clean, live smoke OK |
| T-093 | Tier-1 toll options A/B priced, not built (recommends B) | no code |
| T-094 | twins + post guards live + one-anvil fix (63/zone → 1); epoch 13→14 | t094.bwj OK, entities halved |
| T-095 | widow/gnoll gate closed (zero L9 kills suffered anywhere) | no code |
| T-096 | multi-leg rite demo (8→18 pelts banked) + bless leg proves lane 3/3; skill resets per login | t096a/b/c.bwj OK |
| T-097 | named-elite options (mirror-1010 / Cantor-caster), fixed timers | no code |
| T-098 | decal pressure: peak 210 vs 512 cap, chunking stays closed | no code |
| T-099 | trade-pass human review packet assembled | no code |

## Open director decisions — do NOT implement unasked

1. **L9 climb repeat** (T-090 n=1; logistics card if faster L9 wanted — vial flow/armor, not mob stats).
2. **Toll verdict** (T-093 A/B or combined skill+pelt lever).
3. **Elite cards** (T-097: one per elite when ordered).
4. **Trade-pass human session** (T-099 packet ready; T-033 closes on results).
5. **Tuning answers** (repent scale, lantern price — T-089 defaults = keep).
6. Older: T-ART-09 boss casts beyond Mother, overlay sprite frames (art), NPC 72/73 (no maps), EK ledger (blocked on Marrowgate), T-033 human slice. Never: siege / war / Blood Moon.

## Standing watch items (updated)

- Skill persistence: none by schema (T-096 finding) — bless lane is the soak vehicle for rite proofs.
- Post-graft blade turnover (T-096 observation): single-death probe available if anyone wants it closed.
- Ghoul band: 37.2 s (n=32) noted + dismissed under T-087 discipline (no code path touches it).
- Closed this run: TTK watch, anvil watch, graze watch, decal pressure, widow tuning gate.

## How to continue (standing orders, condensed from AGENTS.md + brief)

- C++20, `-Werror`, no exceptions/RTTI across boundaries, hand-format touched lines (no whole-file clang-format); no new third-party dep without an ADR.
- Server-authoritative; determinism via `sim/rng.h` only; 20 Hz ticks; integers in ticks/tiles in sim+server.
- One card = one sprint = one devlog = one commit (<~400 lines); card in `docs/tasks/done/` with evidence; board updated; GDD numbers changed → update GDD.
- Epoch law: bump `kJournalEpoch` only on content-sim change. Wire law: trailing appends don't bump 237. GDD-vs-shipped → shipped wins + fix GDD.
- Per-command timeout ~30 s: long runs detached, then poll. Standard validation per brief §soak-shape. Val pattern: sqlite-set state, never resume as-is; `/tmp/*.db` reboot-volatile. Bless lane (`--bless user=skill:N,gold:M,id:qty`) for seeded proofs — journaled b-lines, replay-safe.

## Verify on entry

```
cmake --build build -j"$(nproc)"   # warning-free
ctest --test-dir build             # 2/2
./build/tests/bh_tests             # 173/173, 328,624 assertions
./build/server/bh_server --replay-world logs/t094.bwj   # mismatches=0
git log --oneline -3               # 4b4ab47 on top, in sync with origin/master
```
