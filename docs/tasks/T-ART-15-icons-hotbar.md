# T-ART-15 — Item/skill/UI icons + hotbar (filed 2026-09-16, T-156)

**Status:** `open` — renderer + art-lane work, needs a graphical env for evidence. See T-ART-12 header note.

## Context

~60 icons spec'd (`docs/art/40-items-icons.md`); 0 PNGs on disk; bag/vendor/skill panels are text-only. Ruled B0. Implement AFTER T-ART-12 (bigger visual delta first), before T-ART-13/14 per T-156 order (12 → 15 → 13 → 14).

## Scope

Generate the icon set through the aigen pipeline (probe-before-batch, ≤10 gens/session), wire icons into bag rows (incl. T-159 rarity marker + name colour), skill hotbar 1–8, and vendor panels. Renderer + content; no sim/wire/epoch (rarity already rides ItemSlot wire).

## Acceptance

1. Bag/vendor/hotbar screenshots day/night showing icons + rarity chrome.
2. `ctest` green both presets; 00-VERIFY rows → shipped.

## Out of scope

Icon style redesign (art lane); T-159f1 multi-affix display.
