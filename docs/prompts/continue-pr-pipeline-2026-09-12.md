# Continue — the PR execution pipeline (issued 2026-09-12)

This is the operating manual for **executing cards as PRs, session after
session**, and for **merging them** when the director orders it. It assumes
you have read `docs/prompts/continue-building-2026-09-12.md` (the standing
orders: game state, the law, the verification battery, the build queue) —
this file adds the *mechanics* and the *multi-session rules* on top. Where
they overlap, the continuation prompt's law is unchanged; this file extends,
never overrides. Board (`docs/tasks/README.md`) beats both if they drift.

Worked examples cited: the T-116 wave (`merge-wave-2026-09-12.md`), the
T-117 filing (`merge-filing-2026-09-12.md`), and the T-118 renumber wave
(`merge-pr18-2026-09-12.md` — a live cross-session counter collision,
resolved).

---

## 1. The loop — one card, one session, one PR

### Step 0 — sync and claim (NEVER skip)

```
git fetch origin
git log --oneline origin/master -3          # where is master?
gh pr list --state open                     # what's in flight?
grep -n "## Done — T-1" docs/tasks/README.md | tail -5   # live counters
```

- **Counters are claimed from the board on origin/master, not from any
  prompt.** Prompts go stale the moment another session lands a card; the
  board never lies. Next card = highest done-card number + 1; next devlog =
  highest `docs/devlog/NNNN-*` + 1.
- **The collision rule** (learned 2026-09-12, the hard way): two sessions
  branched from the same master both claimed T-117. If your number is taken
  on master by merge time, **your card renumbers, master's keeps the
  number** — do it in a sync merge *before* the PR merges, never rewrite
  master. Renumber *everything that maps to the card number*: the card file
  (`done/T-NNN.md`), the devlog filename + its internal refs, the gate leg
  (`logs/tNNN.bwj` — a byte-identical rename; replay is filename-agnostic,
  re-verify after), the leg script, and the board row. The journal's
  recorded bot names stay as history.
- If your card depends on another open PR, stack the branch and say so in
  your PR body ("based on #N, merge that first") — and expect to rebase.

### Step 1 — author the card

`docs/tasks/T-NNN-slug.md` on your task branch, format: *Context / Scope /
Acceptance criteria (checkboxes) / Tests required / Out of scope*. Copy the
discipline of `done/T-118.md`: the card is written *before* the work and
its acceptance boxes are ticked with evidence.

### Step 2 — branch and worktree

```
git worktree add -b task/T-NNN-slug build/wt/NNN origin/master
```

Branches are `task/T-NNN-slug`, always from master. In an Arena sandbox the
main tree stays on the session branch — all task work happens in linked
worktrees; `build/` is disposable. Never push to `master`; never work on
another session's branch (exception: §2 waves).

### Step 3 — build and test-first

Bootstrap notes for a **fresh sandbox** (they bite — both hit on 2026-09-12):

- The old toolchain may be gone. If `cmake` is missing:
  `python3 -m pip install --user --break-system-packages cmake`
  (PEP 668 blocks plain `--user` installs).
- **A restored repo can be shallow** — `git rev-parse --is-shallow-repository`
  says `true` and cross-branch merges fail with "refusing to merge unrelated
  histories" even when parents visibly match. Fix: `git fetch --unshallow origin`.
- Configure downloads ENet (network needed once); sqlite is vendored
  (`third_party/sqlite-amalgamation/`), so builds are otherwise offline.
  Pre-seed `build/<dir>/_deps/` from an existing build to skip the download.

```
cmake -S . -B build/<dir> -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DBH_BUILD_CLIENT=OFF -DBH_BUILD_SERVER=ON \
      -DBH_BUILD_TESTS=ON -DBH_BUILD_TOOLS=ON
cmake --build build/<dir> -j$(nproc)
```

Tests are written before or with the implementation (doctest in `tests/`,
registered in `tests/CMakeLists.txt`; bot legs via `tools/bots` +
`tools/*_leg.sh` for netcode/sim work).

### Step 4 — the battery (all of it, every PR)

The continuation prompt §5 has the exact commands. Current expected values:
suite **206/206 · 329,022 assertions**, `ctest` 2/2, duel pin
`b273be661b54673a`, `logs/t115.bwj` replay mm=0 (short leg of record) and
`logs/t118.bwj` replay `ticks=12042 sessionCmds=4634 hashes=120
mismatches=0` (freshest long leg). Epoch-guard refusals of t107/t104/t112
are *correct behavior*. If your change touched sim semantics: bump
`kJournalEpoch`, record a fresh `logs/tNNN.bwj`, replay it mm=0, and
`git add -f` it.

### Step 5 — docs with the code

Card moves to `done/T-NNN.md` with evidence pasted in; devlog
`NNNN-slug.md`; board row in `docs/tasks/README.md` inserted **in card
order at the done-junction** (the junction conflicts by construction — when
syncing, keep ALL sections, interleave by number, never drop one).

### Step 6 — the PR

```
git push origin task/T-NNN-slug
gh pr create --base master --head task/T-NNN-slug --title "T-NNN: ..." --body "..."
```

Body = what/why · **full evidence** (suite line, ctest, duel pin, replay
line, soak stats, epoch disposition) · **deviations** · **merge-order
notes**. Skeleton in the continuation prompt §4. Keep diffs < ~400 lines;
split bigger work into follow-up cards.

### Step 7 — leave it OPEN

Default posture: **the director merges.** Your PR sits open with green CI
until they do. Do not self-merge, do not delete your branch while the PR is
open, and say so in the body if the branch is a stack base.

---

## 2. Merge waves — when the director says "merge and execute"

Self-merging happens **only** under an explicit director instruction of the
"make a prompt to merge and Execute it" form. When you receive one:

1. **File the authorization first** (pre-wave record, T-116/T-119 pattern):
   a `docs/prompts/merge-<what>-YYYY-MM-DD.md` on a task branch, pushed
   *before* any merge, stating: authorization quote + scope (which PRs),
   preconditions, procedure, merge styles, rollback (record every head SHA
   first), and the single-use/not-precedent clause.
2. **Verify each target PR** before touching it: CI green at head; body
   carries complete evidence; diff reviewed for scope creep (bots-only
   means `tools/` — a `server/` file in the diff is a stop-and-flag); the
   source session is dormant (last push well past, no WIP signals); no
   children base off its branch.
3. **Sync before merging.** Merge `origin/master` into the PR branch (never
   force-push, never rebase the remote). Resolve board-junction conflicts
   by union-in-card-order. Apply the collision rule (§1 step 0) for any
   number taken on master meanwhile. Push, then re-verify: MERGEABLE,
   CI green on the sync head, and — if anything functional moved — rerun
   the battery locally on the synced branch.
4. **Merge with the right style.** Squash for normal cards. A true merge
   commit only when the graph itself is the deliverable (T-115
   reconciliation precedent). Document the choice in the wave prompt.
5. **Delete branches safely.** `gh pr merge --delete-branch` aborts the
   remote delete when the branch is worktree-checked-out — use plain
   `--squash`/`--merge` and `git push origin --delete <branch>` manually.
   **Never** delete a branch that is the base of open PRs — retarget
   children first via
   `gh api repos/Hansuqwer/BloodHollow/pulls/<n> -X PATCH -f base=master`
   (`gh pr edit --base` fails silently). A raw base-branch delete closes
   child PRs with no retarget (this killed #13; recovery was a replacement
   PR from the same head).
6. **File the evidence**: wave card + devlog + board row + any prompts
   authored, one PR, squash-merged under the same authorization; then
   manual branch delete. Verify master CI green and the queue empty.

If the instruction does NOT have that form, the standing rule applies:
open the PR and leave it open.

---

## 3. Multi-session rules

- Sessions run in parallel on this repo; the shared truth is **origin/master
  + the board + the open-PR queue**. Fetch before every claim and before
  every merge.
- Never push to another session's branch — except a wave-authorized sync
  merge (§2), which is additive (never force).
- Never touch `arena/*` branches (other sessions' session branches), and
  leave stale per-task stubs alone; they are history/rollback material.
- PRs from finished sessions sit open with green CI — that is the *intended*
  end state of their work, not an abandoned one. Check the body's evidence
  and the branch's last-push time before judging dormancy.
- The `app/arena-ai-coding-agent` identity in PR authors is shared by all
  Arena sessions — author identity does NOT tell you which session wrote a
  PR; the branch name and body do.

## 4. The queue (post-T-118 state)

**Track A is CLOSED** — the M3 party-crypt gate ran and the verdict is MET
(5-person mixed-kit party forms, traverses 1→3→5, kills Gravemother — 2
kills in the 300 s leg, TTK <15 s focused; no tuning). Remaining, in the
continuation prompt's order:

| next | card | notes |
|---|---|---|
| T-120 | **B1 weapon-skill persistence** (schema v11) | the T-096 gap; aura grind is fake progression until it lands; likely epoch bump |
| T-121+ | **B2 pledge-lite** | registrar NPC kind + art already shipped (B5); CHA ≥20 + gold; ranks/chat/emblem |
| T-122+ | **B3 Weeping Castle** (mapId 6) | mapgen + validators; gates/Heartstone/throne; content shift ⇒ epoch bump |
| T-123+ | **B4 siege law & scheduler** | biggest card — split it; ticks not wall-clock in sim |
| then | B5 holdings · B6 bot siege army · B7 anti-grief · B8 EK board | GDD §8 |
| interleave | C1 EntityStore SoA · C2 property fuzz · C3 kit completion · C4 docs truth-ups | per continuation prompt Track C |

Blood Moon stays a director call (roadmap vs GDD §10 conflict). Human-only
items (T-033/T-099) are never self-assigned.

## 5. Counters and pins (verify on the board anyway)

Next card **T-120** · next devlog **0082** · journal epoch **20** · wire
**237** · schema **v10** · suite **206/206 · 329,022** · duel pin
`b273be661b54673a` · legs of record `t115.bwj` (mm=0) and `t118.bwj`
(`ticks=12042 hashes=120 mismatches=0`) · commit identity
`bloodhollow-dev <dev@bloodhollow.local>`.

*Filed with the T-119 wave evidence. Execute cards, open PRs, and when the
director says merge — file the prompt, verify, merge, record.*
