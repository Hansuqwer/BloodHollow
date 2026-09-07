# Helbreath (1999, Siementech) — character, VFX & combat-storytelling lock

**Evidence:** `hb-video-thumb.jpg` (koreahb official, `PUK3za0cBag`, archived
2026-09-04) + full storyboard contact `hb-koreahb-official-contact.jpg` (857 s,
dungeon 지하 4층 mass PvP); director link `youtu.be/HbcNadHYQ8o` ("Helbreath
Olympia — Pestilence — Episode 21", 493 s; contact
`hb-olympia-pestilence-ep21-contact.jpg`, key frames `-t0000s…-t0485s.jpg`:
town, Elvine dungeon, forest/field PvP); playhelbreath.com gallery stills
(crusade crowd on grass, 500×375); silva96's open-source client notes (maps
ported to **32×32 ground tiles**, 169×169-tile starting map; sprites exported
per NPC as transparent PNG).

## Answers to the bible's §3.3 questions

**Character/VFX style — what "hand-drawn, chunky, paper-doll-readable"
actually means in pixels.** At native scale an HB human is ~50–56 px tall on
32 px ground tiles (so ~1.7 tiles) — *taller relative to ground than Mir*, which
is why HB piles still parse. Bodies are drawn with a dark 1 px contour and
2–3 tone cel shading, no texture noise; armour reads as **big flat plates of
one colour with a single highlight stripe**; weapons are ~15–20 % oversized
(the two-hander in the thumbnail's inventory is longer than the torso). Skin is
a flat sallow tone. Hair is a single dark mass. → §4.2 palette rule and §4.5
"weapon silhouettes exaggerated ~15 %" are quantified from this.

**15+ player mass-PvP readability.** Thumbnail: 22 bodies in ~12×8 tiles.
What survives: (1) each body's dark contour against a floor whose luma is
~58/255, (2) the **white-blue circular cast rings** under casters, (3) the
red-caps callouts, (4) party names in green. What does *not* survive: gear
detail, faces, which way a sword points. Conclusion: readability is
**contour + ground contrast + text**, never detail. The rulebook's silhouette
budget follows from this.

**Red-caps ability callouts.** "Hell-Fire!", "Detect-Invisibility!",
"Absolute-Magic-Protect!", "Ice-Storm!", "Berserk!", "Mass-Magic-Missile!"
— serif-ish bitmap font, ~14 px cap height at 640×480, **red fill with 1 px
black outline**, hyphenated multi-word, exclamation mark, positioned ~1 tile
above the head, alive ~0.8 s, drawn in world space (they scroll with the map).
T-066 already ships kinds 5/9/10/11/13/14 as floaters with `(255,60,40)` red;
the missing pieces are the bitmap font, the black outline, the hyphenation
convention and a 2 px dark backing plate for night. → §12 callout brief.

**Right-side wooden inventory panel.** Dark-oak plank panel, brass corner
plates, items drawn as **painterly-real objects** (shields, chainmail, potion
flasks, scrolls) at ~40–48 px, free-placed (not a grid) — the era "bag" feel;
weight "( 50 / 50 )" bottom-right; **UPGRADE** button in brass. Ours: keep the
wood/brass and the item-art look, but use a 32 px grid (GDD §7 slots + T-058
durability rows need alignment) — recorded as a deliberate deviation.

**Bottom-left HP/MP bars.** Red over blue, each a bevelled tube with the
number centred *on* the bar (741 / 388), a green XP bar with the floor caption
("던전 지하 4층 (115,258)") beside it. Already the T-052/T-057 layout; the
era-skin at P5 needs the bevelled tube + centred numerals.

**Party names green; floor/depth caption.** Green `(≈120,220,120)` names for
party members in the crowd; the depth caption is white-on-green bar. → §11
karma/party tint set: party green overrides neutral grey but **never
overrides chaotic red** (law is louder than friendship — L1 precedent).

**Olympia footage adds:** (a) torch-lit town at night with visible **warm
pools on cobbles** (t0000) — light radius is painted into the ground around
the source, not just an additive circle; (b) forest PvP frames (t0290–t0385)
show grass at luma ≈ 50 with characters ≈ 130 — again Δ ≥ 25; (c) the
Elvine-dungeon brown-rock tunnels (t0095–t0190) are *lit* brown, not black:
their darkest ambient is still ~40/255. This is the same lesson as Soma's
"permanent daytime" from the other direction: HB never went pitch-black.

## Things NOT to copy

- Free-placed inventory (grid is required by our data model).
- Pure black void outside the dungeon (we letterbox the map edge in a soot tone).
- Fire/ice FX that cover 3×3 tiles opaquely (fails "FX never fully occlude").

## Steal list → briefs

| HB element | Where it lands |
|---|---|
| ~1.7-tile-tall humans, flat cel plates, 1 px contour | player/NPC/mob briefs; `make_sprite` gamma/contrast |
| 15–20 % oversized weapons | §10 weapon silhouettes; Ravager cleaver |
| red-caps bitmap callouts w/ black outline | §12 callout system |
| cast ring under caster | cast anim ground ring (all classes) |
| green party / red chaotic priority | §11 tint set |
| torch pools painted into ground | town/crypt scatter "light pools" |
