# Item icons (32×32 + 24×24, 1× and 2×)

**Status:** design brief only — no art generated (gated on B0 approval).
**Spec:** `docs/art/40-items-icons.md § Item icons`.
**Contents:** Blood Vial 3001, Rusty Shank 2001, Pit Blade 2002, Hide Armor 2101, Bone Plate 2102, junk 4001–4005, gold, reserved: Blackiron Ore 5001, fodder 5101–5103; refine states +0/+5/+9/+10 for the 4 gear rows

Backing colour language (T-066 lock): iron/red = damage · ash-blue = utility · bone-white = heal · gold = holy · violet = curse; plate `#151013` with a 2 px bevel in the family colour.

## Generation prompt
```
"1999 MMORPG inventory/skill icon, 32x32 pixel art, hand-drawn, 1px dark outline, [family] colour backing plate, [glyph line from spec], muted dark palette, no text"
+ NEGATIVE (bible §13)
```
Generate at 8× (256×256), `fit_to_cell` down with NEAREST; the 8 shipped skill icons get a hand redraw at 2× (nearest resize is not accepted for them).

## Delivery checklist
- [ ] icon sheet PNG + atlas JSON (uniform grid, cols × rows listed in the spec)
- [ ] ≤ 32 colours per sheet, palette strip
- [ ] greyscale read at 24×24 hotbar size (no two icons in one family may share a silhouette)
- [ ] `assets/LICENSES.md` rows
