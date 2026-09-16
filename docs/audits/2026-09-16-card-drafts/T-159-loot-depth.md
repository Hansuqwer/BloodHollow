# T-159 — Loot depth: 4 rarity tiers + 5 gear slots + 20 affixes (P1, audit 10/20)

## Context
Audit findings **B7.1 / B7.2 / B7.4 / B7.5**. `grep -rn "rarity|Rarity" server
shared client tests tools` → **0 hits**: the MVP's "4 rarity tiers" does not
exist. `kAffixCount = 10` vs "~40". `ItemDef.slot` is `0 weapon / 1 armor /
2 consumable / 3 junk` — **two equippable slots** against the GDD's eleven. The item table is
~28 rows for a 25-level career. Uniques are fine (12 rows / 4 bosses, tested).
Audit §7 C2 recommends cutting to **5 slots + 20 affixes** but shipping the four
tiers, because tiers are the visible fantasy and slot count is not.

## Scope
- **Rarity**: common/magic/rare/unique with the GDD's 78/17/4.6/0.4 distribution
  rolled at gear-drop time (same place affixes roll), 1–2 affixes on magic, 2–3 on
  rare, uniques keep their fixed rows. Name colouring + a bag-row marker; no new
  art dependency (icons are T-156).
- **Slots**: weapon, armor, helm, amulet, ring (5). Extend `ItemDef.slot`, the
  equip/unequip law, the combat-stat aggregation, and `parseInvBlob` — keep the
  blob's 7-field grammar and **one** shared parser (T-049x law).
- **Affixes**: 10 → 20 with live effect hooks (no flavour-only rows); keep the
  slot-law pins (`test_uniques` pattern) so a mismatched fixed affix cannot ship.
- **Items**: enough steps for L1→25 across 5 slots (target ~45 rows), vendor +
  fence stock updated, drop tables per zone/level band.
- Persistence: additive schema bump only if a column is unavoidable; prefer
  encoding rarity inside the existing blob fields. Journal: if drop semantics
  change, **epoch bump + fresh leg** (coordinate with T-151 so the bump happens
  once).
- OUT: +8..+10 refine, scrolls, crafting, stalls/auction (all OUT-list).

## Acceptance criteria
1. Unit pins: rarity distribution over ≥100k fixed-seed rolls within ±1 % of
   78/17/4.6/0.4; affix count per tier; slot equip/unequip/stat aggregation for
   all 5 slots; blob round-trip with rarity + 5 slots through **login and replay**
   (the T-049x launderer class).
2. Live: a bot wave kills 200 mobs; the drop log shows all four tiers and the
   bag/vendor panels display them (screenshot if a display is available).
3. Replay `mm=0` on a fresh leg recorded with the new drop law; epoch bumped once.
4. GDD §7 amended or an ADR filed for any number that moved (coordinate T-158).

## Tests required
Distribution roll test (fixed seed), blob/persist round-trip, slot-law pins,
economy sanity (vendor buy/sell prices for the new rows).

## Evidence owed at merge
Suite count, distribution output, replay line, screenshots, devlog, board row.
