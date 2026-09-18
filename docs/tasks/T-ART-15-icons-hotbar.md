# T-ART-15 — Item/skill/UI icons + hotbar (STRUCTURE SHIPPED 2026-09-18; art owed)

**Status:** `open` — hotbar UI + icon slots live; the 60-icon art set needs
the pipeline key (none in sandbox). Renderer half done, art half owed.

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
