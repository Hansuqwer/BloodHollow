# Session handoff — 1–10 run (2026-09-09, director-ordered)

*Follows `docs/prompts/session-handoff-continuation-2026-09-09.md` (HEAD `814af0b`).
Director ordered all 10 todolist items executed in order. All 10 landed as
one card = one sprint = one devlog = one commit, all pushed to `origin/master`.*

## Ground state (verified after the run)

- HEAD `b3ca569` — "T-089: tuning validation, pins hold (devlog 0058)",
  pushed to `origin/master`.
- Suite: **167/167 tests, 328,566 assertions** (was 160/160, 328,489),
  `ctest --test-dir build` 2/2, build warning-free (`-Wall -Wextra -Werror`).
- Journal epoch **12** (no bump the entire run — zero content-sim changes).
  Wire `kProtocolVersion` **237** (untouched). `.bhmap` **v2**.
- Latest replay proof: `./build/server/bh_server --replay-world logs/t084.bwj`
  → `[replay] OK ticks=6001 sessionCmds=718 hashes=241 mismatches=0` (and t082 still OK).
- Working tree (leave alone — another workstream's):
  `M tools/atlaspack/bh_mob_sheet.py`,
  `?? docs/prompts/arena-parallel-eval-brief.md`,
  `?? docs/prompts/art-b1-b8-execution.md`,
  `?? tools/atlaspack/b5_build.sh`. Never stage these.
  (Also untracked, mine: `docs/prompts/next-todolist-10-tasks-2026-09-09.md`
  scratch prompt + `logs/t083_runner.log` / `logs/t084_runner.log` runner scraps —
  delete or ignore at will.)
- Git identity for this workstream:
  `git -c user.name=bloodhollow-dev -c user.email=dev@bloodhollow.local`.

## What the run did (all in `docs/tasks/done/` + devlogs 0049–0058)

| card | content | evidence |
|---|---|---|
| T-083 | wander-drift: 2 short legs (16 + 24 deaths), roam-RNG confirmed, no leak | t083a/b.bwj OK |
| T-084 | L8 widow north-edge camp (60,41), bot-only; L8 pair 86 kills / 0 deaths | t084.bwj OK |
| T-085 | base-light verdict B: GDD §9 base-6 → base 0, docs-only | t082 replay unchanged |
| T-ART-11 | refine +5 inventory glow (law + rows + markers), 2 new tests | render-only, replays OK |
| T-ART-09 | ground decal layer (law + surface + boss APIs), 3 new tests | render-only, replays OK |
| T-086 | pilgrim anvil audit: Tier-1 toll (30 pelts/skill 20) exceeds leg income ~6× | no code, correct behavior |
| T-087 | TTK wobble closed: duel tables byte-identical, Ghoul row untouched since Sprint 18 | duel-table-t087.csv |
| T-ART-06 | NPC kinds 67/70–73 engine side (constants + seams + sweep), 2 new tests | wire 237, epoch 12 hold |
| T-088 | bank-road graze audit: 8-leg geography, zero on-route L11, era-correct | no code |
| T-089 | tuning validation: repent/lantern/torch pins hold, 3 director questions framed | 167/167 green |

## Open director decisions — do NOT implement unasked

1. **L8→L9 climb verdict** (new; T-084 opened the camp but 240 s held L8 — needs a 540 s climb + chain evidence if the ladder should push to L9/L10).
2. **L9+ widow/gnoll tuning** (pulled widows survivable but untuned — untuned by design until the climb verdict).
3. **Tier-1 toll retune** (T-086: 30 pelts/skill 20 blocks in-soak grafting — economy design + epoch bump if taken).
4. **Barricade rect tweak** (T-088: priced-not-taken, marginal math + epoch cost).
5. Tuning answers: repent scale, lantern price, trade-pass human slice (T-089 defaults = keep).
6. Older: T-ART-09 boss casts (APIs await callers), T-ART-11 in-world overlays (needs wire field), NPC placement/sheets/portraits (art-side), EK ledger (blocked on Marrowgate), named elites (need design), T-033 human slice. Never: siege / war / Blood Moon.

## Standing watch items (updated)

- Wander-drift watch: keep one more shift of full-length legs; reopen rule in devlog 0049.
- TTK watch CLOSED (devlog 0055 re-pin: 9.7 s duel-med / 8–25 s soak band, n≥30 to claim).
- Pilgrim anvilTries=0: expected (multi-leg progression), no longer a watch item.
- Bank-road graze: expected (wall content), no longer a watch item.

## How to continue (standing orders, condensed from AGENTS.md + brief)

- C++20, `-Werror`, no exceptions/RTTI across boundaries, hand-format touched lines (no whole-file clang-format); no new third-party dep without an ADR.
- Server-authoritative; determinism via `sim/rng.h` only; 20 Hz ticks; integers in ticks/tiles in sim+server.
- One card = one sprint = one devlog = one commit (<~400 lines); card in `docs/tasks/done/` with evidence; board updated; GDD numbers changed → update GDD.
- Epoch law: bump `kJournalEpoch` only on content-sim change. Wire law: trailing appends don't bump 237. GDD-vs-shipped → shipped wins + fix GDD.
- Per-command timeout ~30 s: long runs detached, then poll. Standard validation per brief §soak-shape. Val pattern: sqlite-set state, never resume as-is; `/tmp/*.db` reboot-volatile.

## Verify on entry

```
cmake --build build -j"$(nproc)"   # warning-free
ctest --test-dir build             # 2/2
./build/tests/bh_tests             # 167/167, 328,566 assertions
./build/server/bh_server --replay-world logs/t084.bwj   # mismatches=0
git log --oneline -3               # b3ca569 on top, in sync with origin/master
```
