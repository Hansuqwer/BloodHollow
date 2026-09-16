# 0113 — T-157 M4 re-verdict: horn fixed, gates 2/2, flips 0 (attune gap)

2026-09-16. F1(a) (`BH_GM_NAMES` in drill) + abort-fast assert implemented;
`m4e29b` (12v6 × 750 s): battle joined (12 bands), BOTH gates breached ~tick
5400, attuned 0, flips 0 → M4 still FAIL but the gap moved from drill to
contest: 60 s uncontested attunement never opened vs 6 holders (659 crown
attempts). p99 2.8 ms PASS; replay 15601/2179/157 mm=0 PASS (leg force-added).
Filed T-157-F3 (recommend (a): 16v6 / 900 s first, no sim touch). Driver died
pre-assertions (`wait`-on-reaped under `set -e`, now guarded); verdict from
logs. F1 re-run owes nothing further; F3 owns the flip.
