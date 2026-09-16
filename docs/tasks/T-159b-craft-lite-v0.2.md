# T-159b — L2-lite craft (v0.2, deferred — not MVP)

**Status:** `backlog v0.2` — deferred per BIBLE v2 + audit C2. Do NOT start before Friday-Night Test passes. MVP keeps `docs/02-gdd.md:200-212` Anvil + T-159 (4 tiers / 5 slots / 20 affixes).

## Why deferred
Full Lineage 2 craft (spoil/sweep + materials D/C/B/A + recipes % + dwarven crafter) is L effort, 2–3 sessions, wire+schema+balance. It fights the horror law (dwarven-industrial vs mud/rust/dread) and violates `05-mvp.md:37` change control. Anvil + corpse + auras is on-tone for MVP.

## Scope (v0.2, 6–8 recipes only)
- **Crystalize:** trash junk / shatter overflow → shards at Bonesmith (sinks junk flood, gives reds a fence-adjacent loop). No new UI, just `Bonesmith Twin` verb.
- **8 recipes:** elite/named drops only (e.g. Widow Silk + Blackiron Ore → Vigil gear). Fixed success, gold fee → castle tax sink. Data in `shared/content/recipes.h`.
- **Spoil-flag:** one affix / Cultist curse marks corpse for bonus mats — 1-line They-get-L2-feel, zero new UI. Reuses `kWireKind` corpse path (`T-126 Embers` precedent).

Out: crystal grades, % fail, dwarven class, spoil/sweep separate table — LATER if retention needs it.

## Acceptance
- [ ] `Bonesmith Twin` offers Crystalize: 10× junk → 1× shard, logged, replay `mm=0`
- [ ] 8 recipes craft from inventory near anvil, consume mats+gold, emit item with affix roll
- [ ] Spoil-flag corpse yields +1 mat on kill, logged
- [ ] `shared/content/items.h` rarity/affix/slot counts unchanged beyond T-159

## Depends
T-159 (loot depth), T-151 (wire if craft needs it — avoid), P0s green, Friday-Night ≥70%.
