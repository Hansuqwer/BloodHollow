# 0099 — T-138 pledge-lite rebase (epoch 26, schema v13)

Resumed from `HANDOVER-MVP-SESSION-2026-09-15.md` §1: finished the open
`git cherry-pick -n c0de563` (PR #22 lane) on `task/T-138-pledge-lite`.

Resolutions: `tools/bots/main.cpp` usage-line union
(wander|fighter|pilgrim|campaign|crypt|raider|siege|pledge) + flags union;
kept BOTH siege choreography and pledge ceremony blocks; single siege-aware
fighter gate; wander whitelist gains pledge exclusion. `docs/tasks/README.md`
kept T-118→T-137 rows, appended T-122 row with rebase note.

Repair: staged `persist.cpp` had `loadPledges` spliced inside `loadSiege`'s
row block (missing `}`/finalize/return) — restored, staged.

Epoch 26 (registrar composition), schema v13 (pledge columns + registry),
journal kinds 34–40. Battery + `logs/t138.bwj` leg evidence to be pasted on
the PR. Next: pledge bands + vault follow-ups (Phase P), then Phase A QA-pack.
