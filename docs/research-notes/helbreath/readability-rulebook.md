# Crowd-readability rulebook (bible §3.3 deliverable, §4.5, §15)

Extracted from the koreahb dungeon pile (22 bodies), the Olympia forest PvP
frames, the L1 Void Tower corridor fights and the DE castle brawl. Each rule is
**measurable** so QA can fail an asset without taste arguments. Numbers are for
1× zoom at 1024×768; at 1.5×/2× everything scales with nearest filtering.

## R-LUMA — ground/sprite contrast (the master rule)

`mean luma(sprite opaque body pixels, EXCLUDING the #1a1214 outline ring) − mean luma(terrain under it) ≥ 25` (0–255). The outline is a constant dark frame and never counts against the delta (B0 gate D1, 2026-09-08).
Ancestor measurements (re-audited 2026-09-07 with `qa/luma-audit.json`: 16×16
cell means, median ≈ ground, p90 ≈ sprites/lights — a statistical proxy, not a
segmentation): HB dungeon Δ 66 · HB forest Δ 37–39 · HB town Δ 33 · L1 Tower
Δ 37–42 · L1 town Δ 48 · DE castle Δ 41–66 · Soma swamp Δ 32 · Mir Δ 55–67.
Only an HB tunnel frame (t0095, all rock) falls to Δ 16. The ≥ 25 gate holds;
the earlier eyeball figures ("HB forest Δ≈80, L1 Δ≈55") were overstated on
the sprite side and are struck. Our style tile: outline-inclusive Δ 24,
body-only Δ 43 (rat 50) — see `qa/cell_ravager_S_audit.json`.
Fix order when failing: (1) lower terrain contrast/lift its floor, (2) gamma-lift
the *sprite family palette*, (3) add a 1 px bone rim on the lit edge. Never
alpha or glow hacks (§14.7).

## R-OUTLINE — the contour

- Every sprite: continuous 1 px outline in the `#1a1214` family on the outermost
  ring. No gaps at claws, teeth, staff tips, tails (AI plates break here first).
- The outline is never anti-aliased and never lighter than luma 30.
- Terrain has **no** outlines (painterly lock); furniture/scatter gets one only
  where it must occlude a body (tree trunks, pillars).

## R-SILH — silhouette budget

- Readable at 1× in greyscale against 3 same-tier peers (§15 gate).
- Budget: a 32×48 mob may have **≤ 3 protrusions** beyond its torso ellipse
  (e.g. Gnoll: club, snout, hunch). Bosses ≤ 5.
- One "key shape" per creature that no other mob owns:
  Rat = low teardrop · Bat = W · Ghoul = tall S-curve · Hound = forward wedge ·
  Gnoll = hunch + club · Widow = disc + 8 sticks · Gravecaller = triangle robe +
  censer chain · Sexton = T (halberd) · Elite = bigger T + white bindings ·
  Gravemother = bell dome with torso.
- Humanoid player classes: shoulder width Ravager 14 px > Cultist 11 px >
  Gravecaller 10 px; robe hem length reversed. Class must read from the back.

## R-TEAM — where identity lives

- Faction/alignment/party identity is **only** in (a) the overhead name tint,
  (b) one 6×6 badge glyph beside the name, (c) one gear trim colour ≤ 8 % of
  sprite pixels. Priority: chaotic red > enemy-town > party green > neutral.
- Sprites never recolour for karma (era + GDD §5). Elites: white/gold trim +
  1.25× scale; nameds: unique silhouette + name; never palette-swap alone (§7).

## R-FX — spell effects in a pile

- Duration ≤ 0.6 s for hit FX; persistent effects (Sanctuary, Wither) are
  **ground decals** under the entities, never over them.
- An FX may cover ≤ 40 % of its caster's body pixels at any frame.
- Shape language: bolt = comet with tail · buff = ring under + glyph over ·
  curse = violet glyph over · heal = rising motes · AoE = ground ring first
  (telegraph), burst second.
- Accent ramp only: violet `#6B4A8A` family (highlight `#A884C4`) — era violet,
  desaturated; never design-tool purple — arterial `#8E101C→#D8302A`,
  choir gold `#D9B04A`. Iron/red for damage, ash-blue for utility (T-066).
- The T-066 2-frame hit flash (bone-white → palette) is the *only* time a
  sprite body leaves its palette.

## R-TEXT — callouts

- Red-caps bitmap font, cap height 11 px at 1×, red `(255,60,40)` fill,
  1 px black outline, hyphenated multi-word ("Power-Swing!"), 1 tile above the
  head, life 0.8 s, rises 6 px. Night variant adds a 2 px `#0c0a0a` plate at
  alpha 160 behind the text.
- Damage numbers: white (normal) / orange "!" (crit, existing kind 2) /
  grey "miss"; never red (red is reserved for callouts and chaotic names).

## R-TEXT-2 — name-tag degrade (the crowd rule)

The crowd15 failure is name-tag pile-up, not silhouettes (7 overlaps in a 5-player
pile). When more than 3 overhead name tags would overlap, they degrade to the
**karma-badge glyph only** (lawful / chaotic / neutral). Era-consistent: HB shows
names on hover/target; L1 always-on but shorter. The karma read — the thing PvP
needs — survives; the wall of text does not. (B0 gate, 2026-09-08; client render
change, sibling lane.)

## R-NIGHT — the floor

- Test under the **engine** overlay at hour 02:00–04:00 (alpha 145–150 ≈ 57–59 %),
  not a synthetic 65 % multiply. Pass = silhouette test still passes and
  R-LUMA Δ ≥ 15 after the overlay.
- Every warm light source (torch, anvil, candle cluster, Lantern Spider, "of
  the Vigil") gets a painted ground pool decal 1–2 tiles wide in addition to
  any additive mask the engine grows later.

## R-SCALE — the zoom signature

- Humans ~43 px body in a 48 px cell (≈ 1.34 tile-heights; the B0 "46 px" was
  clipped by the cell top — A7); GDD's "~56 px
  including headroom" = cell + name tag. Common mobs 20–40 px; elites 1.25×;
  Gravemother in 64×64 with the bell dome ≥ 48 px wide.
- Scatter trees/pillars ≥ 2 tiles tall so bodies can *lurk under* them (Soma).

## Automated checks (tools/atlaspack)

`bhpix.count_colours` (≤ 32 opaque), `validate_atlas` (schema v1, 8 dirs,
bounds), `make_style_tile.py` audit JSON (R-LUMA deltas, palette sizes, night
overlay value). A `bh_qa_sheet.py` that renders any packed sheet in the
day/night/grey triptych is the B3 tooling prerequisite.
