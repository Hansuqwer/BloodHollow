# 0087 — The wave that almost merged on a green check that wasn't green (T-126)

Merging a PR is the least interesting thing a session does, until it isn't.
PR #26 carried six gate legs, a law about which kit owns Firebolt, and a
reverted lever, and the authorization to merge it arrived as three words:
*make a prompt to merge*. So the prompt got written first — scope,
preconditions, rollback, and an explicit statement of what the merge does
**not** claim — and then executed line by line.

## The check that lied

The precondition was CI green on the head being merged. The first poll said
it was. It wasn't.

`build-test (macos-latest)` had **failed** at the cmake Configure step, with
Build and Tests skipped after it — and the loop watching it treated `fail`
as "no longer pending" and printed `CI_GREEN`. One wrong grep, and a broken
build would have gone to master under a self-reported pass. The only reason
it didn't is that the raw rows were re-read before merging, which is the
habit that matters: **the check is not what the loop says, it is what the
rows say.**

Then the diagnosis got harder, because GitHub's log endpoint returned `EOF`
on every attempt — six retries and a zip download all came back empty. So
there was no error message to read. What was available instead was a
comparison: the identical source tree had passed macos at `7d47729` ten
minutes earlier, and the only change since was one new markdown file. That
is evidence, not proof, and it was written down as a guess rather than a
finding.

Re-running was refused twice — `gh run rerun --failed` said the run "cannot
be rerun", and the API returned `403 Resource not accessible by
integration`, since this token has no `actions:write`. The only remaining
way to get a second answer was an empty commit, `02dfa1d`, pushed to force a
fresh run. Both jobs passed. Then both passed again on a second run. Same
tree, four green jobs — transient runner fault, and the merge went ahead on
that evidence instead of on the unreadable log.

## The repository that forgot itself

The other incident happened before any of this, and it is the second time.
The sandbox restarted, `.git` rolled back to `818d661` — before PR #25 had
even merged — and the local commit holding the r21 route work, `39dbe3d`,
stopped existing: `git cat-file -t 39dbe3d` answered *Not a valid object
name*. The working tree, which is what actually gets snapshotted, kept
everything: the four route nodes at `main.cpp:312-316`, the revert comment,
all three journals, the renamed handover and devlog.

Recovery is mechanical once you know not to panic: `git fetch`, then
`git reset --mixed` to the remote tip, which moves the ref and the index and
leaves the tree alone. Then re-commit and push. **Never `reset --hard`** —
that is the one command that would have turned a two-minute recovery into
real data loss.

The lesson is not about the recovery recipe, which is now written down in
two places. It is that the work only survived because it had already been
pushed. Twice in one session this branch lost committed work to a rollback,
and both times the difference between an annoyance and a disaster was a
`git push` that happened a few minutes earlier.

## What landed

PR #26 squashed to master as `2845a3d` with its title as the subject, and
`git diff origin/master 02dfa1d` came back empty — identical trees, which is
the check that catches a bad renumber before anyone has to explain one.
Suite **209/209 · 329,058**, ctest 2/2, duel pin `b273be661b54673a`, and all
seven journals replaying `mismatches=0`, including the r17 journal of record
held byte-identical since before the series started
(md5 `b4c0cf3f3255e38071c172cc3a3336bd`).

The source branch was deliberately **not** deleted, which breaks with the
four earlier waves: it is this session's pinned branch.

## The sentence that has to travel with this

Six legs reached the depths. `bossSeen` climbed from 12 to 26. There is not
one Gravemother kill in the record, and no TTK in either direction. What
merged is that the font reach stopped being luck and the crossing got a
staged route — a real, measured improvement, and not a boss verdict. r22's
kiter band is recorded as **inconclusive, not measured red**, because neither
of its two legs ever reached the code that changed.

Anything downstream that reads "M3 gate follow-on merged" as "boss beaten"
is reading a merge notification as a result.
