# Devlog 0088 — T-127 Old Maw uniques ×3 (H3 loot depth, 2/5)

## What

Old Maw (1012) drops three fixed uniques at 4%/row, independent rolls:

| Item | Slot | Stat | Fixed affix | Title |
|---|---|---|---|---|
| Mawsplitter (2201) | weapon | dmg 22 | of Embers (7) | Tooth of the Pit |
| Gullet Plate (2103) | armor | def 13 | of Thorns (5) | The Maw That Keeps |
| Mawfang Shiv (2202) | weapon | dmg 15 | of Greed (9) | Tithetaker |

Pattern for T-128..T-130: `UniqueDropDef` rows in `kUniqueDrops` +
`World::grantUniqueDrop` (fixed item/affix, cap-32 respected, world
broadcast `"<killer> claims <name>, <title>!"` on chatCh 2). Uniques are
found, never stocked (both vendor lanes checked in tests); client renders
placeholder fallback until sheets land.

## Epoch

Per-row `range(1,100)` draws on every 1012 kill shift old journals touching
Old Maw → **22 → 23**. COLLISION FLAG: arena PR #23's title also claims
epoch 23 — T-115 double-19 precedent rules; if #23 merges first holding 23,
the next in-tree card re-bumps.

## Evidence

- `test_uniques.cpp` (new TU, grows with T-128..T-130): 4 cases — row shape
  (affix ≤ kAffixCount, items resolve, titles, 4%), item stats, grant path
  (fixed affix + broadcast), inv-full refusal with zero side effects.
- Full `ctest` 2/2. `validate_links` 0/5. Duel pin unchanged.
- Gate leg `logs/t127.bwj` (5 fighters, 52 kills): replay mm=0.
- Guard: `t126.bwj` (22) refused, exit 4.

## Next

T-128 Red Widow ×3 → T-129 Cantor Vex ×3 → T-130 Gravemother ×3 (= 12/12
MVP uniques). Then the 10 → 40 affix-table follow-up.
