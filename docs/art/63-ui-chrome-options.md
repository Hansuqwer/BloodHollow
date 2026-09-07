# UI chrome — two ways (bible §11, B7 warm-up — A9/A19, drafted both-options)

**Status:** draft. GDD §52 already fixes the *layout* (Helbreath: bottom-left
red/blue bars with numbers on them, right-side wood/brass panels, chat
bottom-left, level always visible, **Soma ornate-silver trim for modal NPC
dialogs only**). What is *not* fixed is the material language of the
non-modal chrome — the choice below. Today's client draws flat black panels
α160–215 with a 1-px blood trim `(150,30,30)` (`game.cpp:917–920, 979`) and
raylib's default font; that is the placeholder both options replace.

## Shared, regardless of option (verified constraints)

- Panel plate stays dark: `#0c0a0a`–`#151013`, α ≥ 170 (today's values work
  under the night overlay because the HUD draws *after* it, `game.cpp:1303`).
- HP `(150,30,30)`, MP `(60,70,160)`, XP bar 6 px, text `(230,210,190)` — keep
  the shipped colours; skin the *frames*, not the semantics.
- All chrome is 9-slice PNGs at 1× with a 2× redraw (not resize) for the 8
  corner/edge pieces; ≤ 16 colours per chrome family.
- Callout font from `export/font/` (D4); UI body text stays the engine font
  until a bitmap-font path exists (T-ART-13 proposal).

## Option A — "Iron & parchment" (Lineage 1 weight)

| Element | Treatment |
|---|---|
| Panel frame | 3-px cold-iron bevel `#3a3e48 / #4a4e58 / #2a2c32`, riveted corners (2×2 px rivet `#8a8e98`), inner 1-px blood line kept |
| Bars | bevelled iron tubes; fill has a 1-px lighter top line; numerals centred *on* the bar (HB) |
| Inventory / shop lists | parchment inset `#8e8674`→`#6a6252` with soot-stained edges; item art on it (HB bag feel) |
| Buttons | iron plate, pressed = 1 px down + darker |
| Tooltip | parchment scrap, torn top edge (4-px sawtooth), black text |
| Bounty board panel | plank + parchment notices — native to this option |
| Anvil modal | iron frame goes **ember** (edge tint `#c8622a`) while pending — the Mir theatre reads strongest here |
| Era read | L1 2001 / HB 1999 — the safest era pass |
| Risk | heavy; at 1024×768 iron frames eat ~10 % more pixels than flat panels |

## Option B — "Soot & bone" (Helbreath flatness, DE austerity)

| Element | Treatment |
|---|---|
| Panel frame | 1-px bone line `#8e8678` inside a 1-px soot line, corners notched (DE stone-arch hint, 3 px) |
| Bars | flat, 1-px bone outline, numerals on the bar |
| Inventory / shop lists | dark oak plank `#3a2e26` with brass corner plates `#8a6a3a` (HB), item art on plank |
| Buttons | flat plank, brass edge on hover |
| Tooltip | black plate α215, bone text, 1-px bone frame |
| Bounty board panel | plank + nailed paper — also native |
| Anvil modal | edge flash only (gold / arterial) — quieter theatre |
| Era read | HB 1999 / DE 1997 — austere; risks reading "modern minimal" if the bone line is too clean → **must** keep 1-px jitter/nicks in the frame art |
| Risk | lighter, faster to produce (~40 % fewer pieces); weaker anvil drama |

## Modal NPC dialog (fixed by GDD: Soma silver) — one design, two portrait framings

| | A19-1 "reliquary" | A19-2 "ledger" |
|---|---|---|
| Frame | silver filigree 9-slice, tarnish `#6a6e78` dominant, highlights `#a9a29a` ≤ 5 % | same filigree, thinner (2 px), more plate |
| Portrait 96×96 | left, in an oval reliquary window, candle key-light from the frame's lower-left | left, square, sits *on* a parchment page with the NPC's name inked under it |
| Text | right, bone on soot, 2 lines + choices | right, black on parchment (only place parchment appears in Option B) |
| Marta sneer swap | portrait only (client swap on `karmaBand==2`) | same |
| Reads best with | Option A (parchment continuity) or B (silver against soot pops) | Option A |

## Recommendation (for the director, not decided)

**Option B for the HUD + A19-1 for modals.** Reasons: (1) the B0 style tile's
sprites are already the busiest thing on screen; flat soot chrome keeps them
so; (2) GDD's "right-side wood/brass" is literally Option B's inventory;
(3) Option A's iron is the *castle* material — spending it on the HUD dulls
the Weeping Castle kit later. Cost of being wrong: the 9-slice sets are ~30
small PNGs either way; switching after B7 is a day, not a batch.

## Widget inventory (≈ 60, GDD §11) — both options share this list

HUD: 2 bar frames, XP bar, level plate, clock (Dawn 30), party frame (5
rows), buff strip 16×16 ×9, target frame, chat plate, log plate.
Panels: inventory (grid 32 px), stat panel, shop list (Marta), anvil modal
(3 states), bounty board (2 states), NPC dialog (modal), death screen plate
("YOU DIED" stays engine text), hotbar 8 slots + cooldown wedge, karma badge
×3, cursor set (default / attack / talk / pick-up / anvil), tooltip 9-slice,
button 3 states, scroll nub, portrait window (A19-1 or -2), letterbox rim
tile (map edge fog band), minimap plate (if the client grows one — none today,
`game.cpp` has no minimap; ⟨design only⟩).
