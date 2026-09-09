# Session handoff — overnight engine shift S20–S25 (2026-09-09)

*Written 2026-09-09 for the morning review. Read `AGENTS.md` first; the
overnight brief (`docs/prompts/engine-overnight-brief.md`) assumes it.*

## Mission accomplished

Worked the overnight queue S20 → S25 in order, one card = one sprint = one
devlog = one commit. S20 (T-033) was already landed; S21, S22a, S22b, S23,
S24, S25 are done below. Suite: **105/105 → 134/134 (328,092 assertions)**,
ctest 2/2, warning-free. Journal epoch **7 → 11**. Everything pushed to
`origin/master` (see `git log` — six overnight commits on top of `a1012a9`).

## Where things stand

| card | title | epoch | suite | replay |
|---|---|---|---|---|
| T-033 (S20) | M2 gate de-QA, programmable half | 7 | 105 | t033.bwj OK |
| T-069 (S21) | Smugglers' fence + one-Marta fix | 7→8 | 112 | t069.bwj OK 14001/5984/141 |
| T-070 (S22a) | Blood Curse + chapel cure | 8→9 | 119 | t070.bwj OK 13601/6093/137 |
| T-071 (S22b) | Torch + lantern + night spawns | 9→10 | 126 | t071.bwj OK 13201/5633/133 |
| S23 audit | Aura III–V already shipped | 10 | — | spot-check on t071 |
| T-073 (S24) | Gate guards + spawn protection | 10→11 | 134 | t073.bwj OK 12801/7321/129 |
| T-074 (S25) | Bot lever: measured, rejected | 11 | 134 | n/a (bot-only) |

Working tree notes: `docs/prompts/arena-parallel-eval-brief.md` (untracked)
belongs to another workstream — untouched. The art stream's uncommitted
`tools/atlaspack/bh_mob_sheet.py` edits + `b5_build.sh` appeared on disk
mid-shift — left alone, never staged. Art commits raced twice; both pushes
were fast-forward (no rebase needed).

## Audit: the inherited Cline session (S21)

Cline was interrupted mid-T-069 with server logic + tests on disk,
uncommitted, suite red. Audit findings: fence core sound (kept); one-Marta
fix is a REAL bug (~425 duplicate vendors — soak entities 544–584 → ~330,
p99 12.9–16.4 → 3.8–5.6 ms); combat hunks (retaliation-on-miss, chase
path-clear) kept as incidental under the same bump; fence seed hoisted out
of the board loop; portal hash test was coupled to the buggy ID layout
(eventual-churn loop now); client crate panel added (F-keys) as a flagged
judgment call. Full story in devlog 0032.

## Judgment calls pinned overnight (director review, in one place)

1. T-069 buy-lane routes **by item**, not proximity (disjoint stocks).
2. T-069 client panel exceeds render-only scope (else `fenceBuy` is dead for
   humans). Panel keys moved F6–F8 → F8–F10 in T-071.
3. T-069 combat hunks kept under the fence epoch (documented, not carded).
4. T-070: Gravecaller has NO bolt kit (source = shared boss-bolt path);
   Bless unscaled (no heal); vfx-14 already HASTE; OwnStats +u16, version
   stays 237 (S16 precedent).
5. T-071: lantern 150g at Marta (no brief basis); torch + lantern both at
   Marta; kUseItem covers use (no new command); `night_ghouls` at (36,26);
   held targets kept at dawn; Spawn/Delta +u8 light, version stays 237;
   bhmap DID bump (1→2).
6. T-073: post sizes (2/1200), retaliation-despite-protection, fence open to
   wanted, expiry stand-down, "gallows-bound coin" lines. Epoch 10→11
   (brief's 11→12 assumed an S23 bump that never came).
7. T-074: pack-gate drop REJECTED by data (32→104 deaths), reverted to v5c.
   Nothing shipped. Next pit: retreat 5→4, repeats, L1-naked.

## Standing watch items (notin's broken, don't "fix" blind)

- Mid-band TTK wobble (Ghoul 8.9→11–14, Hound 18→23–57, Gnoll 21→66–320 on
  n≤12): four legs of noise shape, owned by T-074 discipline.
- Bank-road corner night-graze (12–13,38–40): relocated gravecallers catch
  roaming melee at night; campaign route clean every leg.
- Pilgrim anvilTries=0 (parts/gold gates) since T-033.

## Open director decisions (do NOT implement unasked)

- **L8→L9 step-up** (widow_glade/barricade adjacency) — awaiting director,
  options in devlog 0030 scope boundary.
- Human trade-pass UX review (T-033 remainder).
- Karma repentance at the chapel (deferred from T-070).
- All seven judgment-call pins above.

## How to verify this shift

```
cmake --build build -j"$(nproc)"   # warning-free
ctest --test-dir build             # 2/2
./build/tests/bh_tests             # 134/134, 328,092 assertions
./build/server/bh_server --replay-world logs/t073.bwj   # mismatches=0
```
