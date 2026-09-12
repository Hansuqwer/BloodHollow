# Execution prompt — merge wave 2026-09-12 (T-116)

**Filed & executed in one session** (repo pattern: PR #8's critical-review
prompt). **Merge authority:** the standing AGENTS.md rule "never merge your
own PRs — the human reviews everything" is **explicitly overridden for this
wave by director instruction** ("Make a prompt and merge. Execute prompt.",
session 2026-09-12). The director has had every PR's evidence in front of it
(local verification reports + green CI on both matrices for all seven PRs).
No other session may cite this prompt as precedent for self-merging.

## Objective

Land the post-T-103 review era on `master` in one verified sequence and
leave the PR queue empty. End state: master at `kJournalEpoch = 20` carrying
the union of both review lineages + B5 art + the T-113/T-114 logistics
verdict, with a replaying gate leg of record (`logs/t115.bwj`).

## Preconditions (abort the wave if any fails)

1. All seven PRs open and `MERGEABLE`: #8, #9, #10, #11, #12, #13, #14.
2. CI green on every PR (build-test × {ubuntu, macos}).
3. Epoch ladder sound: only #9 (19) and #14 (20) carry bumps; #14 contains
   #8+#11's content; after the wave master reads 20 exactly once.

## Ordered steps

1. **#12** (T-113 docs → master) — squash-merge, delete branch.
2. **#13** (T-114 bots+legs; base auto-retargets to master when #12's branch
   is deleted) — verify base=master and MERGEABLE, then squash-merge
   (subject "T-114: …"; its tree-diff vs master is T-114-only — #13 carries
   #12's commit but that content is already on master), delete branch.
3. **#9** (T-104 wave → master) — squash-merge, delete branch. Brings the
   vendored sqlite: master builds offline from here on.
4. **#10** (B5 art; head carries #9's commits) — verify the diff collapsed
   to B5-only, squash-merge, delete branch.
5. **Sync #14's branch**: merge master into `task/T-115-lineage-reconcile`
   and resolve the board junction (docs/tasks/README.md done-sections in
   card order T-111…T-115), push. Without this, #14 conflicts with #12/#13
   at the same junction.
6. **#14** (T-115 reconciliation; base auto-retargets to master when #9's
   branch is deleted) — verify base=master and MERGEABLE, then merge with a
   **true merge commit** (the reconciliation graph is the point), delete
   branch.
7. **Close #8 and #11** as superseded (their content is fully inside #14;
   supersession comments already posted on both).

## Post-merge verification (all must pass)

- `git fetch origin`; build headless **offline** on the new master
  (vendored sqlite proves itself here).
- Suite: 206/206 cases, 329,022 assertions, 0 failures; ctest 2/2.
- `bh_duel --selftest`: win=1 ticks=41 hpEnd=82 hash=b273be661b54673a.
- `bh_server --replay-world logs/t115.bwj` → `mismatches=0` (epoch 20).
- Epoch refusals by contract: t107.bwj (18), t104.bwj (19), t112.bwj (19).
- PR queue empty (except this wave's own evidence PR).

## Record

File the execution evidence as card `docs/tasks/done/T-116.md` + devlog
0078 + board row, and squash-merge that evidence PR to close the wave.

## Rollback

- Merges: `git revert -m 1 <merge-sha>` (or revert the squash sha) on
  master, push via a revert PR.
- Closed PRs: `gh pr reopen 8` / `gh pr reopen 11`.
- Deleted branches (#9/#10/#12/#13/#14): recreatable from their PR head
  shas (recorded in the evidence card) or from the merge commits.
