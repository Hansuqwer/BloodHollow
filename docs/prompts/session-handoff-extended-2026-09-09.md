# Session handoff — extended engine shift S26–S36 (2026-09-09)

*Written 2026-09-09, following
`docs/prompts/session-handoff-overnight-2026-09-09.md`. The overnight brief
ended at T-074; this shift executed `docs/prompts/engine-extended-brief.md`
end to end.*

## Mission accomplished

S26 → S36 in order, one card = one sprint = one devlog = one commit.
Suite: **134/134 → 160/160 (328,489 assertions)**, ctest 2/2,
warning-free. Journal epoch **11 → 12** (one bump: T-079). Everything
pushed to `origin/master`.

## What landed, in order

| card | title | epoch | suite | replay |
|---|---|---|---|---|
| T-ART-01/02/08 (S26) | point filter, zoom snap, maps 4/5 | 11 | 135 | n/a (render) |
| T-ART-04 (S27) | anim-state hook | 11 | 137 | n/a (render) |
| T-ART-05/07/10 (S28) | atlas table, party tint, anchorY | 11 | 140 | n/a (render) |
| T-075 (S29) | karma repentance | 11 | 147 | t075.bwj OK 12801/7483/129 |
| T-077 (S30) | retreat 5→4 measured, rejected | 11 | 147 | n/a (bot-only) |
| T-078 (S31) | guard-murder consequences | 11 | 150 | t078.bwj OK 12801/6929/129 |
| T-079 (S32) | refine +4 to +7 | 11→12 | 152 | t079.bwj OK 12801/7984/129 |
| T-080 (S33) | trade transaction log | 12 | 154 | t080.bwj OK 12801/7389/129 |
| T-081 (S34) | durability on death −5 | 12 | 156 | t081.bwj OK 12801/7536/129 |
| T-082 (S35) | Cultist Purify chan 9 | 12 | 160 | t082.bwj OK 12801/7391/129 |
| T-076 (S36) | base-light DECISION, no code | 12 | — | — |

Ledger: T-ART-03 done-superseded by T-071; T-ART-06 partial (68/69 ship);
T-ART-09/11 parked (decals need the layer; glow needs +5, now unblocked by
T-079). Both bot levers measured red and reverted (pack-gate 32→104,
retreat 32→92); tree holds v5c + a 3-line comment.

## Judgment calls pinned this shift (director review, in one place)

1. S26: three micro-commits; `bh_tests` links raylib PRIVATE (snapZoom
   header needs it); atlas-test absence noted stale, not invented.
2. S27: durations half-cadence derived (8/8/4t, die held); contact-first;
   walk/idle fallback until B3+ lands.
3. S28: 1011 renders fallback hero (sheetless, stated); enemy-town tint a
   documented no-op; test law extracted to `mobSheetPaths`/`overhead.h`/
   inline `animAnchorY` (link-driven, stated).
4. S29: repentance +20/72000t derive from whitening pins; wanted refused.
5. S30: revert kept a 3-line comment (flagged, zero behavior change).
6. S31: deficit vs victim L15; shared `markWanted`; whitening fires blindly
   on guards (L15 nets −299, pinned in test).
7. S32: GDD rates above frozen shipped rows; T-ART-11 unblocked.
8. S33: fixed `logs/trades.log` (no wall-clock near sim); canonical
   lower-id order; suite-hygiene fix (swap test repointed, file ignored);
   replay duplicates byte-identical lines (dedup trivially).
9. S34: drops-then-wear order; all bands rust.
10. S35: Cultist unlock 6 (Ironskin parity); Mend-mirror gates; no key
    (6–8 precedent verified: raw SkillUse bytes).
11. S36: non-binding B recommendation.

## Standing watch items (carried + new)

- Mid-band TTK wobble: Ghoul swung 8.5–24.7 across legs — noise shape,
  owned by T-074 discipline. (Ghoul touched the 8.9 pin twice: noise
  confirmed, not closed.)
- Wander-death drift (24→146 across two shifts): shape says roam-RNG
  (spread killers/grounds, no L15), but monotonic — prime suspect is
  live-entity growth (330→378) widening aggro coverage. Revisit if it
  persists two more legs.
- Bank-road corner night-graze; pilgrim anvilTries=0. Unchanged.

## Open director decisions (do NOT implement unasked)

- **L8→L9 step-up** (still; options in devlog 0030).
- **Base light A/B** (new; devlog 0048).
- Human trade-pass UX; karma-amount tuning (+20/1h are derivations);
  lantern price (150g, overnight pin); siege / war / Blood Moon (never).
- T-ART-09 decals, T-ART-11 glow (unblocked, unscoped), NPC kinds
  67/70–73, EK ledger (blocked on Marrowgate), named elites (need design).

## How to verify this shift

```
cmake --build build -j"$(nproc)"   # warning-free
ctest --test-dir build             # 2/2
./build/tests/bh_tests             # 160/160, 328,489 assertions
./build/server/bh_server --replay-world logs/t082.bwj   # mismatches=0
```

Working tree (no action): arena brief untracked, art-brief untracked,
`b5_build.sh` untracked, `bh_mob_sheet.py` modified — all another
workstream's, never staged.
