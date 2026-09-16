# T-159 — Loot depth: 4 rarity tiers + 5 gear slots + 20 affixes (SHIPPED 2026-09-16)

Source: audit card draft `a1bd867:docs/audits/2026-09-16-card-drafts/T-159-loot-depth.md` (P1, audit 10/20). Implements the audit §7 C2 cut: 5 slots + 20 affixes + 4 tiers.

## Scope shipped

- **Rarity** (`shared/content/items.h`): `kRarityCommon/Magic/Rare/Unique` 0..3, rolled in `World::killMob` gear-drop path 78/17/4.6/0.4 (GDD §7). Magic/Rare roll 1 affix 1..20; Common 0; uniques keep fixed rows. `InvSlot.rarity` + `InvSlotWire.rarity`, 8-field blob (`...:refine:rarity;`, legacy 7-field defaults 0, one shared parser — T-049x law kept).
- **Slots**: `ItemDef.slot` 0 weapon / 1 armor / 2 helm / 3 amulet / 4 ring / 5 consumable / 6 junk. Equip/unequip, armor-def aggregation (armor+helm sum), repair-all, refine, vendor/fence junk lanes, client click routing all moved (`>1`→`>4`, `==2`→`==5`, `==3`→`==6`). +9 rows: Scrap/Graveguard/Hollow-Warden helms, Bone-Charm/Grave-Lodestone/Marrow-Talisman amulets, Iron-Band/Ossuary-Ring/Seal-of-the-Hollow rings. Uniques marked rarity 3.
- **Affixes 11..20** with live hooks (no flavour-only rows): 11 Hollow +3 flat · 12 Grave-touched +1 OOC regen · 13 Crypt −10% incoming · 14 Marrow +3% lifesteal · 15 Pall +2 acc +1 evd (`effEvd`) · 16 Boneyard +5% crit (deterministic extra roll) · 17 Dirge +4 at night · 18 Husk +2 def · 19 Tithemaster +15% kill gold · 20 Last Rites +8 <20% hp (in `equippedWeaponDmg`, display == dealt).
- **Wire**: `ItemSlot += u8 rarity` (`messages.md`), protogen base 201→202 → `kProtocolVersion` 241→242, server pack + client unpack. Old clients refused (reason 4 class).
- **Epoch**: 28→29 (RNG-stream + damage-law change). Fresh leg `logs/t159.bwj` (8 fighters ×30s, ticks=641 cmds=417 hashes=6, replay mm=0). Guard: `logs/epoch28.bwj` refuses exit 4.
- **GDD**: `docs/02-gdd.md` §7 affix list amended to the shipped 20.

## Deviations recorded (director-visible)

1. Affix count per tier is 1 for magic AND rare (card asked 1–2 / 2–3). `InvSlot` holds one affix; multi-affix needs a schema change — deferred, not silent.
2. Item rows +9 (≈37 total), not ~45 with per-band drop tables. Vendor/fence stock updated for slot presence; full L1→25 band plan deferred to T-162.
3. Rarity name-colouring / bag-row marker: wire ships, client render deferred to T-156 icon/panel work.
4. Boneyard proven by slot-law pin + code path (statistical crit-rate test deferred as seed-fragile); Pall evd + Last Rites pinned deterministically (`effEvd`, `debugWeaponDmg`).
5. Live 200-kill all-tiers drop log NOT produced (headless; server stdout carries no per-drop line). Distribution pinned by 100k fixed-seed unit test; drop-log instrumentation is a follow-up.

## Tests

- Headless: **314/314 (2,330,419 assertions)**, `ctest --preset headless` 2/2. New: Pall-evd, Last-Rites, Boneyard-slot-law, helm/amulet/ring resolve + vendor-stock + equip-agg, rarity blob/100k-distribution, ItemSlot-rarity roundtrip. Fixed en route: world.h duplicate decl, test_uniques deref, Crypt re-attack loop, Marrow sipped-pattern, inv_blob 8-field literal.
- Leg: `logs/t159.bwj` replay `mismatches=0`; `logs/epoch28.bwj` refused exit 4.
- `linux-gcc` full build not run here (raylib/X11); headless is the gate per T-154.

## Out of scope (kept)

+8..+10 refine, scrolls, crafting (T-159b v0.2), stalls/auction, multi-affix items, per-band drop tables, client rarity chrome.

## Review note

Diff is ~500 lines vs the <400 guideline (AGENTS.md exception: single-card atomic unit — data table + hooks + pins cannot split without breaking the build; tests are 60% of it).

## Follow-ups filed

- T-159f1: multi-affix items (schema) + per-band drop tables + drop-log line + client rarity chrome (colour/marker) + statistical Boneyard pin.
