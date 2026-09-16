# T-165 — PR queue: merge the #49→#51 stack, cherry-pick #23's tests, close superseded (P1, audit 16/20)

## Context
Audit finding **H1 / H2 / H2b / H8**. Eight PRs are open against a tree whose
history is a single squashed commit. Triage on 2026-09-16:

| PR | Content | State | Verdict |
|---|---|---|---|
| #49 | T-142 wire class/sex + `atlasForPlayer` (+148/−7, 12 files) | MERGEABLE | **merge first** (P0-6 dependency) |
| #50 | T-R-LUMA provisional accept (base = #49's branch) | MERGEABLE | merge after #49 |
| #51 | art probe 2/10 + closes T-104/T-112/T-ART-B5 (base = #50's branch) | MERGEABLE | merge after #50 |
| #22 | T-122 pledge-lite (epoch 22) | UNKNOWN | **close-superseded** — pledge core landed via the T-138 cherry-pick (proved live: pledge `t122clan`, vault 500, ranks restored) |
| #23 | T-123 Weeping Castle (epoch 23, stacked on #22) | UNKNOWN | **do not close blindly** — carries `tests/test_castle.cpp` + `tools/t123_castle_leg.sh`, neither of which exists in master; cherry-pick both, then close |
| #30 | T-126 affix v2 (epoch 22) | UNKNOWN | **close-superseded** — `kAffixCount = 10` and `test_affix_v2.cpp` are compiled in master |
| #27, #28 | docs/handover + art plates | UNKNOWN | director call: rebase for the docs, or salvage the art files and close |

Also: devlog ids collide across branches (#49 ships `0107-…`, #50 `0108-…` while
master's newest is 0106 — this audit files **0109**), and `tests/test_siege_stub.cpp`
is on disk but uncompiled (deliberately, per `2bd471d` — see Scope).

## Scope
- Merge the #49 → #50 → #51 stack **in order**, verifying CI green at each step
  and re-running the replay leg after the wire change (#49 adds a message →
  `kProtocolVersion` moves; check whether an epoch bump is owed and say so).
- Cherry-pick `tests/test_castle.cpp` + `tools/t123_castle_leg.sh` from #23 onto
  master (adapt to epoch 27), wire the test into `tests/CMakeLists.txt`, run it.
- Close #22, #30 with a comment naming the master commit/behaviour that supersedes
  them; rule on #27/#28 with the director.
- Delete the stale `shared/protocol/gen/` duplicate (build generates into
  `${CMAKE_BINARY_DIR}/generated`) or document why it is kept.
- `tests/test_siege_stub.cpp` is **already ruled** by `2bd471d` ("removed from
  build, file kept as evidence") — do not re-decide it. Add a one-line comment at
  the top of the file and a note in `tests/CMakeLists.txt` so a future agent does
  not re-add it to the build.
- Add a devlog-id reservation rule to `AGENTS.md`/board README (parallel branches
  must not collide) and a `git add -f` note for new `logs/*.bwj` legs.
- **This card is executed by the director or an agent explicitly authorised to
  merge**; the audit session did not merge anything.

## Acceptance criteria
1. Open-PR count drops from 8 to ≤3, each remaining PR with a stated next action.
2. Master after the merges: CI green both OSes, suite count ≥ pre-merge count,
   `logs/t146.bwj` (or its epoch-successor) replays `mm=0`.
3. `tests/test_castle.cpp` compiled and passing on master.
4. Class/sex visible in-client (PR #49's own AC) with a screenshot.
5. Board + devlog numbering consistent, no duplicate ids.

## Evidence owed at merge
Merge order log, CI URLs, suite counts before/after, replay line, devlog, board row.
