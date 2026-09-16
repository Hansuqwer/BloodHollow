# 0114 — T-157 F3(a) failed: 16v6 still denied at attunement

2026-09-16. `m4f3a` (16v6 × 900 s @ epoch 29): battle 1, gates 2/2 (both by
t137a__11), attuned 0, flips 0 → M4 still FAIL. p99 4.8 ms PASS; replay
18601/3435/187 mm=0 PASS (leg force-added). (a) exhausted: 12v6/750 s and
16v6/900 s both die at the 60 s uncontested window vs 6 holders — and sim
constants are unchanged since the epoch-25 3/3 M4, so this smells structural,
not numerical. F3 now a director call: (b) tune attune law (sim change +
re-proof) or (c) accept the gate shape. Driver died pre-assertions again
(environmental; verdicts from logs). Cards: `done/T-157.md`, `T-157-F3`.
