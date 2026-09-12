# 0078 — The wave, executed (T-116)

The prompt said: land the whole review era in one sequence and leave the
queue empty. Seven PRs, four squashes, one true merge, one accidental
closure, one replacement PR, and two board-junction conflicts later,
master is the reconciliation graph.

What made it work was that every judgment call had been made in advance:
T-115 carried the pre-resolved lineage union (epoch 20, union tryAnvil,
all three test TUs), so the wave's only real decisions were merge styles
(squash for everything except #14 — the reconciliation merge is a merge
commit because the two-parent history is the deliverable) and the order:
design (T-113) → hardening (T-104) → its consumers (T-114, B5) → the
reconciliation on top, children retargeted before any base-branch delete.

The one casualty: deleting #12's branch raw-closed child PR #13 (GitHub
closes children of a deleted base without retargeting), recovered by
opening #15 from the same head. The rule that fell out: retarget first,
delete last — and when a branch must die with open children, expect a
replacement PR, not a reopen.

The subtle moment was #8 closing itself one second after #14 merged. Not
a bug: GitHub marks a PR merged when its head becomes reachable from the
base, and the reconciliation merge — by design — pulled #8's lineage
into master's ancestry. The supersession we'd promised in comments
happened by graph reachability instead of a close button. #11 needed the
button (its base was a session branch, so no reachability check fired).

Verification on the final master (0355367): offline build from vendored
sqlite (the #9 wave's contribution proving itself one PR later), 206/206
(329,022 assertions), ctest 2/2, duel pin `b273be661b54673a` unchanged,
`t115.bwj` replaying mismatches=0, and the epoch guard refusing all
three stale legs (t107 at 18, t104 and t112 at 19) with the contract
message. CI green on both matrices for every merge commit. Queue empty.

Two lineages, one ladder, epoch 20. The review era is closed.
