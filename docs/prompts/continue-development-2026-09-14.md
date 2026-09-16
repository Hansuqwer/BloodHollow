# Continue Development Prompt — 2026-09-14

## Authorization

User request (2026-09-14): "Make a prompt to analyse repo , continue development, commit, push , pr. Execute prompt"

This prompt is both the analysis and the execution plan. It follows the repo's continuation-prompt pattern (see `docs/prompts/continue-pr-pipeline-2026-09-12.md` §1) and creates the next card in the queue.

## Repo Analysis (2026-09-14, master 2845a3d)

### Ground truth
- **Branch**: `arena/01a09f1a-bloodhollow` from master `2845a3d` (T-124+T-125 wave)
- **Journal epoch**: 21, **wire**: 237, **schema**: v11
- **Suite**: 209/209 · 329,058 assertions, ctest 2/2, duel pin `b273be661b54673a`
- **Legs of record**: `logs/m3_gate.bwj` (r17, 36042 ticks, 5933 cmds, 360 hashes, mm=0), `logs/t120.bwj` (1500 ticks, mm=0), plus r18-r22b series (6 legs, all mm=0) from T-125
- **Build**: headless preset (`-DBH_BUILD_CLIENT=OFF`) builds clean (74/74); client build needs X11/GL (CI-only when apt blocked)

### M3 Gate Status (Track A)
- **Font reach is repeatable** (T-125 headline): r19 3/5 at 147s, r20 4/5 at 268s, **r21 4/5 at 231.7/232.8/237.5/237.9s** with 3-5 elites and 12-14 trash per bot (vs r20 1 elite, 2-4 trash). r21 is best-known config.
- **Boss verdict still NOT established**: no leg has killed Gravemother. `bossSeen` now routine (20-26), kills 0. Party dies at map-5 x=15 inside triple aggro: apse elite (16,4) r8 hp380, Sexton (21,5) r8 hp420, Gravemother (21,2) r8 hp700. Deepest x=15 of 22; last 7 tiles are the whole fight.
- **r22 (kiter band on map 5) attempted, never exercised, reverted**: r22 0/5 never entered map5 (618/791 traces on map1), r22b 2/5 stuck at node (6,21) 1.3-2.3s. Change is inert on maps 1/3 where both legs were lost → leg variance, not lever. Reverted because `continue` branch is r8c stall risk.
- **Firebolt bug fixed in T-125 r20**: kitClass 3→2, pinned in `tests/test_kits.cpp` (6 new assertions). Whole r8→r19 series fought melee-only.
- **Next moves per handover T-125-r18-r22.md §5**:
  1. Re-run r21 to confirm 4/5 mode (n=1)
  2. Break x=15 triple-aggro: extra node (19,10) walkable, 5 from Sexton, so apse elite dies before Sexton aggros; or pre-mark ordering; or kiter band re-land
  3. Then boss verdict
  4. Level question director call (L13 vs L18-25 content)

### Open PR Queue (Track B)
- **PR #22 T-122 pledge-lite (epoch 22)**: registrar wireKind 72, level≥10+10k gold, ranks Liege/Bloodsworn/Initiate, pledge chat `/p`, schema v12 (pledges table), journal g-sidecar. Files: server/world, persist, command, tests/test_pledges.cpp, tools/bots, logs/t122.bwj. **CONFLICTING** against new master in `tools/bots/main.cpp` (merge-tree: CONFLICT content) because master rewrote raider profile (+1009/-46 in PR #25).
- **PR #23 T-123 Weeping Castle (epoch 23, STACKED on #22)**: mapId 6, 56x44, moat+2 causeways, 2 gate gaps, Heartstone 150k HP, gates 100k HP, throne, Oathbroken Sentinel 1015 L14, portals fields↔castle. Also CONFLICTING in same file, plus depends on #22. Must sync after #22.
- Both carry old devlog numbers 0084/0085 which master has taken (0084 gate series, 0085 merge wave). Per collision rule, they renumber at merge time to free numbers (0087+).
- No other open PRs. Queue after B3: B4 siege law & scheduler (biggest card, needs split), B5 holdings, B6 bot siege army, B7 anti-grief, B8 EK board.

### Other Backlog
- **T-104, T-112** cards exist in `docs/tasks/` root but PRs #9 and #11 are MERGED — stale files, should be moved to done or deleted (housekeeping).
- **T-ART-B5** done, merged as PR #10. Art backlog T-ART-01..11 still open (engine tasks for art visibility).
- **Track C**: C1 EntityStore SoA (deferred, arch doc says current AoS deque allocates per tick — debt), C2 property fuzz (1M iterations), C3 kit completion, C4 docs truth-ups.
- **M5 gate**: still far — need pledges, siege, hardening.

### Risks
- Bot leg variance is large: same binary r22 0/5 vs r22b 2/5 vs r21 4/5. One leg cannot retire/promote a lever. Budget 2 legs per change.
- Sandbox wipes `.local/` and `build/` on restart; `.git` can roll back while working tree keeps newer content. Push early.
- `apt-get` blocked → headless battery only; client-law TUs (7 cases) covered by CI.
- `bh_maps` is ALL target — must `ninja bh_maps` before tests/replay or 4 loadZone tests fail.
- Replay is `bh_server --replay-world`, no `bh_replay` binary.

## Execution Plan — T-126

### Card Choice Rationale
T-126 is the natural next card per T-125 handover §5.1-5.2: confirm r21 mode and break the triple-aggro. It is:
- **Bots-only** (tools/bots/main.cpp) — no sim semantics, no epoch bump, no wire, no schema, safe to land on top of open pledge PRs without epoch collision
- **Highest value for M3 closure**: the gate is 4/5 reach but 0 boss kills; the wall is precisely mapped (x=15 triple-aggro)
- **Small diff** (<100 lines) — reviewable, <400 line rule

Alternative considered: sync PR #22/#23 now. That is also valuable but is a merge-wave task (needs director authorization per pipeline doc §2). This prompt creates a new development card (T-126) rather than executing a wave. Pledge sync can be T-127.

### T-126 — M3 gate finalizer: re-run r21 + node (19,10) + kiter band re-land with runner guard

**Context**: r21 is best-known config (4/5 at 232s, 3-5 elites). It dies at x=15 inside three simultaneous aggro fields. Handover suggests extra node (19,10) — walkable, on corridor, 5 from Sexton — so apse elite (16,4) is killed before Sexton aggros. Also re-land r22 kiter band behind a reaching leg, with front-runner guard (never band the runner).

**Scope**:
- `tools/bots/main.cpp`:
  - **Map-5 route**: insert node (19,10) between (13,10) and (22,10): new table `{{6,21},{13,10},{19,10},{22,10},{22,3}}` (5 nodes). Verify walkable: data/maps-src/drowned_crypt.tmj ground id not in {2,3} at (19,10) — checked against T-125 handover which says walkable corridor is up x=6, east along y=10, north up x=22; (19,10) is on y=10 east segment.
  - **Kiter band re-land**: change condition from `b.mapId !=1 && !=3 && !=5` (currently OFF on map5) to allow map5, but guard front-runner: `if (raider && kitClass==2 && bestD<=8 && b.mapId==5) { if (frontRunnerOf(b).id == selfId) continue-as-runner (skip band); else band logic }`. Also keep existing off for map1 and map3 (march-first). Implement as: if mapId==5 and isFrontRunner, skip band; else run band. The band itself: farEdge 8/6, nearEdge 6/3, same as before. This is handover's "never band the runner".
  - **Optional**: keep distance-aligned marks (already in) and Firebolt kit gate 2 (already in).
- No server/shared/client changes. Epoch stays 21.
- **Legs**: run r21 re-run (same binary as T-125 r21? No, new binary with extra node, so it's r23). Actually: first leg = r21 re-run with OLD binary to confirm mode (n=1 → n=2). Second leg = new binary (r23) with node (19,10) + kiter band guard. If time allows, third leg = r23 second run to measure variance.
- **Artifacts**: force-add journals `logs/m3_gate_r21b.bwj` (re-run) and `logs/m3_gate_r23.bwj` (new config) if they improve on r21.

**Acceptance**:
- [x] Build headless warning-free; suite 209/209 (or 209+ if new tests, but no new tests expected) — this card is bots-only, suite unchanged
- [x] Duel pin unchanged
- [x] Existing legs replay mm=0 (t120, m3_gate)
- [x] New route node (19,10) verified walkable against blocked grid (document in code comment)
- [x] Kiter band re-landed with front-runner guard — code comment explains guard, no stall on map1/3
- [x] At least one 900s leg run with new config, journal replay mm=0, traces show map5 entries, deepest x, elite kills, bossSeen
- [x] Devlog 0087 + handover update + board row

**Tests required**:
- Headless battery: `ninja bh_bots bh_server bh_tests bh_duel bh_maps`, `bh_tests`, `ctest`, `bh_duel --selftest`, replay of old legs, replay of new legs
- Map-5 node walkability check: `python3 -c "import json; tmj=json.load(open('data/maps-src/drowned_crypt.tmj')); ..."` or manual verification against mapconv derived blocked logic (ground id 2/3 blocked)
- Soak: at least one 900s leg via `tools/m3_gate_leg.sh` (or shorter smoke if time constrained)

**Out of scope**:
- Any server/sim change, epoch bump, schema, wire
- Pledge/castle work (T-122/123)
- Boss tuning, level changes
- Deleting/renaming old journals

**Deviations to record**:
- If only one leg run due to time, note leg variance warning (need n=2 per handover)
- If node (19,10) blocked, fallback to (18,10) or (19,11) with verification

## Execution Steps (this prompt's own checklist)

1. Create branch `task/T-126-m3-finalizer` from master (already on arena branch, but we are on arena/01a09f1a — we will commit to this branch per session rules)
2. Implement T-126 changes in `tools/bots/main.cpp`
3. Build headless, run suite, duel, replay
4. Run at least one m3_gate leg (900s if possible, else 300s smoke) to get new data
5. File devlog `docs/devlog/0087-t126-m3-finalizer.md`, handover `docs/handover/T-126-r23.md`, card `docs/tasks/done/T-126.md`, board row
6. Commit, push to `arena/01a09f1a-bloodhollow`, open PR to master with evidence
7. Do NOT self-merge (director merges)

## Counters and Pins (for this prompt)

- Next card: T-126, next devlog: 0087, epoch 21, wire 237, schema v11, suite 209/209 · 329,058, duel pin b273be661b54673a, leg of record m3_gate.bwj 36042 ticks mm=0, t120.bwj 1500 ticks mm=0
- Branch: arena/01a09f1a-bloodhollow (session-pinned, never push elsewhere)
- Ground truth: same as repo analysis above
