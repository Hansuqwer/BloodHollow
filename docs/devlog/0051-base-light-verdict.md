# 0051 — Base light: B wins by economy (T-085)

T-085 executes the T-076 decision as Option B. One line in `docs/02-gdd.md` §9; zero code.

## Why B (director-ordered, recommendation-confirmed)

- The torch economy (8g sink, Marta's 6th stock, 6000t burn) was balanced around base 0 (devlog 0034). Option A spends an epoch bump (12→13) plus a soak leg to delete a working sink and thin the torch to duration-only — all cost, no playtest evidence of darkness fatigue.
- Shipped-wins rule (brief §standing): GDD-vs-shipped conflict keeps shipped values and fixes the GDD. This is exactly that case.
- Reversal path stays open: if night playtests report fatigue, Option A is fully priced in devlog 0048 (<40 lines + 3 test pins + fresh leg) and becomes its own sprint.

## Proof of no change

No binary touched: `cmake --build` no-op, suite 160/160, `logs/t082.bwj` replays identically (12801/7391/129 mm=0). T-076 closes from DECISION to EXECUTED.
