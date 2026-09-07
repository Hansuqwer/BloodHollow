# Myth of Soma (2001, Digital Bros / Grigon) — the terrain lock

**Evidence:** web captures archived as `soma-web-1..4.jpg` (+ `soma-web-contact.jpg`): gamepressure.com original 640×480 captures (swamp with broken
aqueduct columns in still black water, root-choked forest edge, torch stumps —
the exact scene 01-research §6b describes); mythofsoma-legoc.vip feature page
(quoted below); myth-of-soma.fandom.com server history; 01-research §6b field
note (director, 2026-09-04).

> LEGOC 2025 client notes: "☀️ **Permanent Daytime** — No more squinting
> through pitch-black nights" · "Isometric Rendered High resolution Graphics"
> · "Dynamic weather and day/night cycles affecting gameplay".
> (mythofsoma-legoc.vip, fetched 2026-09-07)

A live private server shipping *both* "day/night cycles affect gameplay" and
"permanent daytime" in the same feature list is the whole Soma lesson in one
line: the mechanic survived, the visual was amputated because it was unreadable.

## Answers to the bible's §3.5 questions

**Painterly, pre-rendered ground.** The 640×480 swamp capture: ground is a
continuous painted plate (no visible diamond grid), mid-frequency detail
(root ridges, mud sheen, leaf litter) with **no hard outlines**, colour range
olive-brown → cold blue-black water, cell-median 55, p90 87 (Δ 32; `qa/luma-audit.json` `soma_web_3`) — the
first-draft "floor ≈ 35, ceiling ≈ 140, Δ≈50" was overstated and is struck.
Sprites over it are ~55–60 px tall with dark contours. The
painterly surface *is* low contrast; the sprites carry the contrast. This is
the split lock justified from a single frame.

**Dense rooty forests / swamp mud / broken aqueducts over still water.**
Objects are **big**: a column stump is ~1.5 tiles tall, the aqueduct arch ≈ 4
tiles wide, the tree canopy at the frame top covers ~6 tiles. Water is *flat*
— a dark blue-black plane with a few painted highlight strokes and column
reflections, no animation. Scatter is *composed*: roots grip the shore line,
columns march in a broken row, torches sit at path bends. → §6.6 "no animated
water", §6 "scatter composed so monsters lurk in shadow pools" — and the
style-tile tree at 112 px (2 tiles) tall is the first proof of the zoom.

**Tall, non-chibi humanoids — the zoom signature.** Soma's human ≈ 58 px on a
tile roughly 48×24 → ~2.4 tile-heights, the tallest ratio of the five
ancestors (HB ≈ 1.7, L1 ≈ 1.6, Mir ≈ 1.5). Our engine: 46 px body on a 32 px
tile-height = 1.45 — **closer to Mir than Soma**. Two options were weighed:
(a) shrink tiles (breaks `.bhmap`, A*, everything), (b) push bodies to the
cell ceiling. Chosen: **(b) 46 px body + tall scatter** (trees/pillars ≥ 2
tiles) which gives the *perceived* Soma zoom without touching the sim. Flagged
in `VERIFY.md` as a director call if the style tile reads "too small".

**Ornate gothic-silver UI trim.** Bottom bar: silver filigree frame, HP/MP
**orb** (red/blue halves) bottom-left, a potion belt of glass vials, skill
icons in silver bezels. We adopt the filigree *only* for modal NPC dialogs (GDD
§2) so the HUD stays HB; vial art for F1–F8 side-slots is Soma-derived.

**The night lesson (locked).** Soma's night was a near-opaque blue multiply;
LEGOC's fix was removal. Our engine already caps at alpha 150/255 (59 %) in
`daynight.cpp` and the style tile at hour 02:00 keeps both sprites and the
tree legible (see `style_tile_256_night.png`). Every terrain piece in B1/B2 is
rendered through `bhpix.night_floor()` before delivery; failures get a
palette lift, never a brighter overlay.

## Things NOT to copy

- Soma's saturated magenta spell sparkles on the aqueduct (visible in the
  capture) — reserved ramp only.
- HP orb as the main HUD (HB bars are the lock; orb reads Diablo).
- Any anime-adjacent portrait style from the 2001 marketing.

## Steal list → briefs

| Soma element | Where it lands |
|---|---|
| continuous painted plate, no grid | `cut_diamond()`; per-biome painted plates |
| root clusters gripping shorelines; column rows | fields/crypt scatter composition |
| flat black water with painted highlights | Drowned Crypt floor, fields puddles |
| 2-tile-tall trees/pillars | scatter scale rule R-SCALE |
| silver filigree modal frame | §11 NPC dialog trim |
| glass-vial potion belt | F1–F8 side-slot icons |
| "permanent daytime" cautionary tale | R-NIGHT gate, engine cap kept |
