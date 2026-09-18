# 0123 — ADR-0016 wave (T-159f1.1/.2: multi-affix + 45-row tables, epoch 31)

## What
- Multi-affix: `InvSlot` +`affix2`/`affix3`; blob 8→10 fields (legacy tails
  default 0/0, no SQLite migration — schemaless TEXT). Magic = 1 + 50% 2nd,
  Rare = 2 + 50% 3rd, distinct by mod-shift (no extra RNG draws). `hasAffix`
  matches any field; Whet/Embers/Hollow/Dirge/Last-Rites/Warding/Husk/Leech/
  Marrow generalize via `slotHasAffix`. Uniques keep one fixed affix.
- Tables: kGearDrops 4→45 rows banded by mob level; EVERY matching row rolls
  independently (T-127 per-row precedent — first-match-only retired).
- Wire 241→242 (ItemSlot +affix2/affix3, protogen base 202→203, T-142 class);
  client bag row shows up to three affix names. Epoch 30→31 + fresh gate leg
  `logs/t159f1.bwj` (8×60s + relog, replay mm=0, guard refuses `wave2.bwj`
  exit 4). GDD §7 rarity line amended.
- Stale-comment fix: the wave-2 "Wire 242→244" line was wrong (generated read
  241 pre-bump) — corrected in place.

## Evidence
Suite 388/388 · 2,332,023 (7 new TUs in `test_multiaffix.cpp`: 10-field
round-trip, legacy compat, hasAffix-any, Whet+Embers=21 / Warding+Husk=15
stacks, 2000-kill roll statute Magic 1–2 / Rare 2–3 / distinct / Common 0,
45-row table law) + 2 updated pins (`test_affix_v2`, `test_inv_blob`
canonical shapes). ctest 2/2 both presets, headless 380/380, validate 0/6.
Live leg drop lines carry the new `affix=0+0+0` shape. T-159f1 items 1–2
CLOSED; T-159f1 fully done.
