# Session handoff — continuation (2026-09-09, after the extended shift)

*Follows `docs/prompts/session-handoff-extended-2026-09-09.md`. The extended
brief (`docs/prompts/engine-extended-brief.md`, S26–S36) is fully drained.
Nothing is in flight: tree work is committed and pushed, suite is green.*

## Ground state (verified this session)

- HEAD `033e32d` — "T-082 + T-076 + extended handoff (devlogs 0047/0048)",
  pushed to `origin/master` (no `HEAD..origin/master` delta).
- Suite: **160/160 tests, 328,489 assertions**, `ctest --test-dir build` 2/2,
  build warning-free (`-Wall -Wextra -Werror`).
- Journal epoch **12** (last bump: T-079 refine +4–+7). Wire
  `kProtocolVersion` **237**. `.bhmap` **v2**.
- Latest replay proof: `./build/server/bh_server --replay-world logs/t082.bwj`
  → `[replay] OK ticks=12801 sessionCmds=7391 hashes=129 mismatches=0`.
- Working tree (leave alone — another workstream's):
  `M tools/atlaspack/bh_mob_sheet.py`,
  `?? docs/prompts/arena-parallel-eval-brief.md`,
  `?? docs/prompts/art-b1-b8-execution.md`,
  `?? tools/atlaspack/b5_build.sh`. Never stage these.
- Git identity for this workstream:
  `git -c user.name=bloodhollow-dev -c user.email=dev@bloodhollow.local`;
  always `git pull --rebase` before push (art commits race on `master`).

## What the last shift did (S26–S36, all in `docs/tasks/done/` + devlogs 0038–0048)

| card | content | replay |
|---|---|---|
| T-ART-01/02/08 | point filter, zoom snap {1,1.5,2}, maps 4/5 | n/a (render) |
| T-ART-04 | anim-state hook, contact-first, walk/idle fallback | n/a (render) |
| T-ART-05/07/10 | atlas table, party tint, anchorY | n/a (render) |
| T-075 | `/repent` +20 karma / 72000t CD, wanted refused | t075.bwj OK |
| T-077 | retreat 5→4 measured red (32→92), reverted | n/a (bot-only) |
| T-078 | guard-murder stain vs L15 + shared wanted path | t078.bwj OK |
| T-079 | refine +4–+7 (65/50/35/25), epoch 11→12 | t079.bwj OK |
| T-080 | canonical trade log, lower-id leads | t080.bwj OK |
| T-081 | death wear −5, floor 0, drops-then-wear | t081.bwj OK |
| T-082 | Cultist Purify chan 9, Mend-mirror gates, cleanse-only | t082.bwj OK |
| T-076 | base-light A/B priced, no code (recommends B) | — |

Prior shifts: overnight S21–S25 (T-069 fence/epoch 8, T-070 curse/epoch 9,
T-071 torch+lantern+nightOnly/epoch 10, S23 aura audit, T-073
guards+protection/epoch 11, T-074 lever rejected). Phase-3 queue per
`docs/prompts/phase3-remainder.md` is drained. Judgment-call pins for all
11 extended sprints are collected in the previous handoff §"Judgment calls"
(devlogs 0038–0048 hold the full reasoning).

## Open director decisions — do NOT implement unasked

1. **L8→L9 step-up** (oldest; options in devlog 0030).
2. **Base light A/B** (new; devlog 0048: A = GDD-literal base 6, torch
   duration-only, epoch bump; B = keep shipped 0 + one-line GDD fix;
   non-binding recommendation B).
3. T-033 human slice (trade-pass UX — human-only).
4. Karma-amount tuning (+20/1h are derivations), lantern price (150g
   overnight pin), human trade-pass UX.
5. T-ART-09 decals, T-ART-11 glow (now unblocked by T-079, unscoped), NPC
   kinds 67/70–73, EK ledger (blocked on Marrowgate), named elites (need
   design). T-ART-03 done-superseded by T-071; T-ART-06 partial (68/69).
6. Never: siege / war / Blood Moon.

## Standing watch items

- Mid-band TTK wobble (Ghoul touched the 8.9 pin twice — noise confirmed,
  not closed; T-074 discipline owns this comparison).
- Wander-death drift 24→146 across two shifts: shape says roam-RNG, but
  monotonic — prime suspect is live-entity growth (330→378) widening aggro
  coverage. Revisit if it persists two more legs.
- Bank-road corner night-graze; pilgrim `anvilTries=0`. Unchanged.

## Candidate next work (pick one — all need a director order or a new card)

- Await director verdict on (1)/(2) above — both are priced, neither is scoped.
- Wander-drift investigation (2+ fresh short-soak legs + entity-count/aggro
  correlation) — would be T-083 if ordered.
- T-ART-09 or T-ART-11 scoping/implementation — needs art-side input first.
- Pilgrim `anvilTries=0` probe — small, bot-only, never scheduled.

## How to continue (standing orders, condensed from AGENTS.md + brief)

- C++20, `-Werror`, no exceptions/RTTI across boundaries, `clang-format`
  before commit; no new third-party dep without an ADR.
- Server-authoritative; determinism via `sim/rng.h` only; 20 Hz ticks;
  integers in ticks/tiles in sim+server.
- One card = one sprint = one devlog = one commit (<~400 lines unless the
  card says otherwise); card moves to `docs/tasks/done/` with evidence;
  board (`docs/tasks/README.md`) updated; GDD numbers changed → update GDD.
- Epoch law: bump `kJournalEpoch` only on content-sim change; stale journals
  refuse by contract. Wire law: trailing appends don't bump 237 (S16
  precedent). GDD-vs-shipped conflict → shipped wins + fix the GDD line.
- Per-command timeout ~30 s: long runs detached —
  `(setsid ./build/<thing> ... > logs/<run>.log 2>&1 < /dev/null &)` then
  poll. Standard validation: fresh DB `/tmp/tNN.db`, server
  `--soak-secs 640 --record-world logs/tNN.bwj`, 14-bot grinder mix
  (wander ×5 / fighter ×4 / pilgrim ×3 / campaign ×2, `--secs 540`);
  then `--replay-world` must read `mismatches=0`.
- Val pattern (resumed-DB): copy chain DB + sqlite-set state (level/xp/
  map/pos) — never resume as-is (false death-loops); `/tmp/*.db` is
  reboot-volatile.

## Verify on entry

```
cmake --build build -j"$(nproc)"   # warning-free
ctest --test-dir build             # 2/2
./build/tests/bh_tests             # 160/160, 328,489 assertions
./build/server/bh_server --replay-world logs/t082.bwj   # mismatches=0
git log --oneline -3               # 033e32d on top, in sync with origin/master
```
