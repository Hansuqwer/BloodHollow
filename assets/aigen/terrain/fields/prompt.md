# prompt.md — Fields of the Overflow tileset

**Status:** DRAFT prompt package (B0.5 A1). No plate generated. Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

## Prompt

```
PREFIX   = "pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic, seamless tileable, even overcast light, muted desaturated palette only (mud brown, stagnant green, soot grey, bone off-white, black-blue water), no objects, no characters, no text, no border, luma range 30-140"
SUBJECT  = "see BRIEF.md / spec"
PALETTE  = "per zone palette anchors in docs/art/10-terrain.md §6.1–6.5"
NEGATIVE = "bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette"
```

Full prompt = PREFIX + ", " + SUBJECT + ", palette: " + PALETTE + " --neg " + NEGATIVE

## Expansion
- plates: one 4× painterly generation per terrain id in the BRIEF list, prompt = PREFIX_TERRAIN + subject line from docs/art/10-terrain.md
- process: box-reduce → blur 0.8 → contrast ≤ 0.75 → floor lift (D3) → family quantize ≤ 32 → cut with bhpix.cut_diamond; QA with bh_qa_sheet.py --kind plate
- edges: bhpix.edge_masks() gives the 8 masks per pair; bhpix.blend_edge() composes a first cut, then hand-paint the boundary (roots, mortar, algae)
- objects/furniture: sprite PREFIX + #00FF00 plate; anchor at feet; light pools as separate 96×48 / 128×64 decals

## Hand-fix list (check in this order after quantize)
- no outlines on ground; outline only trunks / pillars that occlude bodies
- plate mean luma 45–70, min ≥ 24 (bh_qa_sheet plate gates)
- no accent hue anywhere on terrain; warm light only in pool decals

## Cell / sheet
see BRIEF.md

## Runs
_(none yet — one line per generation: date · model or "undisclosed" · seed if known · which dirs · accepted y/n)_
