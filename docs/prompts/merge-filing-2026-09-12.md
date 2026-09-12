# Filing merge — continuation prompt + session handover (issued 2026-09-12)

*Companion to `merge-wave-2026-09-12.md` (the wave it follows). Same rule:
single-use, this session only, explicitly **not precedent** for any other
session's self-merging.*

## Authorization & scope

Director's instruction, this session: **"Make a prompt to merge and Execute
prompt."** Scoped to **one docs-only PR** that files the post-wave standing
orders onto `master`:

- `docs/prompts/continue-building-2026-09-12.md` — the continuation prompt
  (next-card references bumped T-117→T-118 / devlog 0079→0080 because this
  filing consumes them).
- `docs/prompts/session-handoff-postwave-2026-09-12.md` — this session's
  handover to the next build session.
- `docs/prompts/merge-filing-2026-09-12.md` — this file (the authorization
  record).
- `docs/tasks/done/T-117.md` + devlog `0079-continuation-prompt.md` + board
  row in `docs/tasks/README.md`.

**No code, no wire, no schema, no content, no epoch change** (stays 20).

## Preconditions (abort the filing if any fail)

1. `master` at `c8d8087` or a docs-only delta since. If any *code* moved,
   re-run the continuation prompt's §5 verification battery on master before
   merging.
2. PR queue empty except this PR.
3. CI green on this PR (docs-only diff, but CI runs the full client build on
   ubuntu+macos regardless — that is the smoke gate).

## Procedure

1. Branch `task/T-117-continuation-filing` from `master` in a linked
   worktree (the main tree stays on its session branch).
2. Commit the five docs with the `bloodhollow-dev` identity; push.
3. Open the PR against `master` with what/why, evidence, deviations, and
   merge-order notes (none expected — queue is empty).
4. On CI green: **squash-merge** (authorized here), then a **manual** remote
   branch delete (`git push origin --delete task/T-117-continuation-filing`)
   — the branch is worktree-checked-out, so `--delete-branch` would abort
   the remote delete (the #12 lesson).
5. Verify master CI green and the PR queue empty afterward.

## Rollback

Revert the squash commit on `master`. Nothing else exists to roll back.

## Record

Card `done/T-117.md`, devlog 0079, and the board row ship inside this same
PR — there is no separate evidence round (unlike the wave, the filing and
its evidence are one commit-set).
