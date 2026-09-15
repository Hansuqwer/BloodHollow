# Devlog 0089 — T-128 trio uniques ×9 (H3 loot depth, 3–5/5 COMBINED)

## What

One combined card for Widow + Cantor + Gravemother (the T-127 pattern
repeated 3× — separate cards would burn two extra epochs/legs for zero
review value; T-129/T-130 stay free). The MVP 12-unique set is complete:

| Boss | Item | Affix | Title | Rate |
|---|---|---|---|---|
| Widow 1013 | Widow's Needle 2301 (20) | Focus 6 | The Huntress Answers | 4% |
| Widow 1013 | Silkwoven Shroud 2104 (12) | Mending 10 | Woven From Hunger | 4% |
| Widow 1013 | Red Widow's Kiss 2302 (17) | Leech 3 | Drink, Dear | 4% |
| Cantor 1014 | Cantor's Quill 2303 (19) | Embers 7 | The Last Verse Burns | 4% |
| Cantor 1014 | Vigil Cope 2105 (12) | Vigil 8 | The Choir Keeps Watch | 4% |
| Cantor 1014 | Vex Nail 2304 (16) | Whet 1 | Keen As Doctrine | 4% |
| Gravemother 1009 | Tithehook 2401 (28) | Greed 9 | The Mother Collects | 6% |
| Gravemother 1009 | Sepulcher Plate 2106 (16) | Ox 4 | Born Heavy | 6% |
| Gravemother 1009 | Caulblade 2402 (24) | Leech 3 | First Blood, Again | 6% |

Slot law enforced in tests (armor 2/4/5/8/10, weapon 1/3/6/7/9). Boss 6%
is a judgment call (L14, fixed timer). All found, never stocked.

## Epoch

New per-kill draws on 1013/1014/1009 → **23 → 24**. Fresh leg
`logs/t128.bwj`.

## Evidence

- `test_uniques.cpp` +3 cases (per-boss counts/rates, slot-law sweep over
  all 12 rows, item stats, per-boss grant spot-checks, never-stocked sweep
  over all 9). Full ctest 2/2. validate_links 0/5. Duel pin unchanged.
- Gate leg (5 fighters, 60 kills): replay mm=0. Guard refuses t127 exit 4.

## Next

H3 loot is DONE (12/12 uniques, 10-affix table). Remaining H3-adjacent:
the 10 → 40 affix-table follow-up (filed, post-MVP-gates). Next: Phase H4
(Blood Moon + EK + L19 oath).
