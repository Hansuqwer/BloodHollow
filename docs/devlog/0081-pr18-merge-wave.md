# 0081 — The collision, and the playbook it produced (T-119)

Two sessions, one number. The other session took the continuation prompt's
original "next card T-117" at face value and built the M3 party-crypt gate
on it — correctly, per the text it had. Mine had consumed T-117 on master
in between for the prompt filing. The PR arrived CONFLICTING, which is the
system working as designed: the board is the counter authority, and git
flagged the divergence better than any process doc could.

The resolution wrote itself into the repo's memory: master keeps its
number, the incoming card renumbers, and everything that maps to the card
number moves with it — card file, devlog, gate leg (a byte-identical
rename; the journal replays fine under any filename, re-verified:
`ticks=12042 hashes=120 mismatches=0`), leg script. The renumber ran inside
a sync merge on the PR's own branch, pushed additively, re-CI'd, and only
then squash-merged. PR #18 is now T-118 on master, and its verdict stands:
**M3 is met** — the five-person party clears to the Gravemother and kills
her. Phase 3's milestone criterion, formally measured at last.

The session also paid two environment taxes worth recording: a restored
sandbox can come back **shallow** (merges fail with "unrelated histories"
even when parents visibly match — `git fetch --unshallow` fixes it), and a
fresh sandbox may have no cmake (pip `--break-system-packages` installs
it). Both are now in the pipeline prompt's bootstrap notes.

And that's the real deliverable of this card:
`docs/prompts/continue-pr-pipeline-2026-09-12.md` — the loop (claim →
card → worktree → battery → PR), the merge-wave protocol, the multi-session
rules, and the queue with Track A closed. The next session reads the
continuation prompt for the law and this one for the mechanics, takes
T-120 (weapon-skill persistence — the aura grind stops being fake
progression the day it lands), and the pipeline turns.
