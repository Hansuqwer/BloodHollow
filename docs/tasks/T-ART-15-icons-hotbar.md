# T-ART-15 — Item/skill/UI icons + hotbar (ITEM TOP-10 SHIPPED 2026-09-19; batch 2 owed)

**Status:** `open` — skills + top-10 items live and wired; second item batch
(helms/amulets/rings/junk/ore) needs the next session cap.

## Context

~60 icons spec'd (`docs/art/40-items-icons.md`); 0 PNGs on disk; bag/vendor/skill panels are text-only. Ruled B0. Implement AFTER T-ART-12 (bigger visual delta first), before T-ART-13/14 per T-156 order (12 → 15 → 13 → 14).

## Scope

Generate the icon set through the aigen pipeline (probe-before-batch, ≤10 gens/session), wire icons into bag rows (incl. T-159 rarity marker + name colour), skill hotbar 1–8, and vendor panels. Renderer + content; no sim/wire/epoch (rarity already rides ItemSlot wire).

## Acceptance

1. Bag/vendor/hotbar screenshots day/night showing icons + rarity chrome.
2. `ctest` green both presets; 00-VERIFY rows → shipped.

## Out of scope

Icon style redesign (art lane); T-159f1 multi-affix display.

## Progress (2026-09-18 — structure landed, art owed)

- Skill hotbar (bottom-center, online): follows the live input page
  (plain 1-6 / Shift+1-8 / Ctrl+1-5, same layout as `handleInputOnline`),
  per-slot lock state from `kits.h` unlocks at the hero's kit+level (server
  re-validates; display only), §11 family-colour plates, req level shown
  when locked. Shot `tart15_hotbar.png` (Cultist L1: Resurrect dimmed L20).
- Bag rows: rarity icon plate ahead of the text (placeholder rect; rarity
  always reads even when glow/aura win the text colour). Rarity marker +
  name colour were already live (T-159f1.3).
- **Owed:** the ~60-icon art set (aigen pipeline, probe-before-batch ≤10 —
  NO key in this sandbox, `.env` has no POLLINATION entry); cooldown sweep
  needs cooldowns on the wire (not carried — file separately if wanted);
  vendor-panel plates; 2x icon redraws.

## Progress 2 (2026-09-18 — skill icons shipped + wired, key found)

- Key discovery: `.env` carries `POLINATION_API_KEY` (note the single-L
  spelling — the MCP `POLLINATIONS_API_KEY` env name never matched it).
  Validated against the gateway (`/v1/models` lists) and used via header
  auth; the key never touches logs or docs.
- 9/10 session gens (flux.1-schnell, seeds in `prompt.md` §Runs): ch1–ch8 +
  ch10 raws under `assets/aigen/icons/skills/raw/`. Probe-first passed
  (cleaver accepted); batch deviations handled in post, no regens: shadows
  throughout, ch4 dark-mottled bg, ch7 white bg (adaptive border-median key
  + 1px fringe erode), ch3 micro-text (vanishes at 32px), ground discs kept
  as icon bases.
- Post (inline script, brief-compliant): NEAREST 28px fit on 32 cell,
  MEDIANCUT ≤32 (sheet holds 31), pipeline `#151013` plate + 2px family
  bevel, NEAREST 24px + palette strip, greyscale silhouette check (distinct
  per family). Sheet `skill_icons.png` + JSON cell map (channel→rect).
- Client: `Game::ensureSkillIcons` (lazy, missing files → placeholders);
  hotbar draws real icons + outlined key numerals (legible over art) + lock
  dim kept. Shot `tart15_hotbar_icons.png` (crop verified: all 6 plain-page
  icons + L20 lock).
- LICENSES rows appended (model disclosed: flux.1-schnell).
- **Owed:** item/UI icon batch (next session, 1 gen held + fresh cap);
  vendor-panel plates; 2x hand redraws (brief wants them for the 8 shipped
  skills — programmatic NEAREST stands in); cooldown sweep (wire).

## Progress 3 (2026-09-19 — item top-10 shipped + wired, 10/10 fresh cap)

- 10 gens (flux.1-schnell, seeds in `icons/items/prompt.md` §Runs):
  2001/2002/2003/2101/2102/2501/3001/3003/3004/gold. Deviations in prompt.md
  (prop bases, thin shank, micro-glyphs) — all read at 32px, no regens.
- Post: adaptive key, NEAREST 28px fit, MEDIANCUT exactly 32, pipeline plate
  + family bevel (weapons iron, armor ash, vial bone, torch/lantern/gold
  gold), 24px + greyscale check (distinct). Sheet `item_icons.png` + JSON
  keyed by itemId.
- Client: `Game::ensureItemIcons` + `drawItemIcon` (lazy; rarity-plate
  fallback where uncovered). Bag rows: 14px icon + shifted text. Vendor +
  fence rows: 14px icon + shifted text. Shot `tart15_vendor_icons.png`
  (F1–F6 + F12–F14 icons live, rest fall back cleanly).
- **Owed:** batch 2 (~15: Graveguard/Bone-Charm/Lodestone/rings/junk/ore/
  fodder + leftovers), vendor plates done, 2x redraws, cooldown wire.
  Note: a new gateway key (`sk_…9L`, provided 2026-09-19) is on file with
  the director — endpoint reachable, auth unproven (models list is public);
  next batch's first gen is the real test. It is stored NOWHERE in the
  tree; `.env` untouched.
