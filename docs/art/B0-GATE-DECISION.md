# B0 Gate Decision — style-lock APPROVED + D1–D12 rulings

**Date:** 2026-09-08 · **Base:** `origin/master @ 0468644`
**Authority:** director sign-off, executed by delegate (director: "execute the B0
style-tile gate + D1–D12; I trust your recommendation"). Every ruling below is
reversible by the director; the reasoning is recorded so an override is cheap.
**Method:** all mechanical gates re-measured against source; the tile and the
decision board were **viewed** (day / engine-night / greyscale + the D1–D5
variant panels), not taken on faith.

---

## Part 1 — The B0 style-lock gate: **APPROVED**

The tile (`style-tile/style_tile_board_3x.png`) passes the §0/§15 gate. Signed.

| Gate | Evidence | Result |
|---|---|---|
| Terrain = Soma painterly | mud/furrow ground + dead-willow root cluster, diamonds cut from one plate | pass |
| Characters = Helbreath hand-drawn | Ravager + rats, hard 1px `#1a1214` outline, visible Bayer dither | pass |
| Combat storytelling = HB red-caps | "FIREBOLT!" callout, name tags, hit-flash frame | pass |
| Grade = L1 + DE accents | scene ≤30% saturation; only saturated px = callout + ember + blood decal | pass |
| Night rule (Soma lesson) | night panel fully legible at α145 — tree, both sprites, callout all read | pass |
| Greyscale silhouette | tree / Ravager / both rats nameable at 1× | pass |
| Gore language | matte `#3A080C` decal, not sparkle | pass |
| Era test | no bloom, no neon on terrain, no gradients, hard outline, dither — sits next to the ancestor captures | pass |

**Two conditions carried into production (not blockers):**
1. **Plate min-luma 22.1 < 24** — fix at B1 by clamping the floor to 24 in
   `make_ground_tiles` (no new plate needed). Tracked as a B1 task.
2. **R-LUMA gate definition corrected** (see D1). Under the corrected rule the
   Ravager passes at **Δ42.7**; the old "23.7 marginal" reading was an artifact
   of counting the dark outline ring against the delta.

The gate is **open**. B1–B8 may start, subject to the sequencing note in Part 6.

---

## Part 2 — The four style-tile README notes (D1–D4)

- **D1 — Ravager R-LUMA Δ23.7 vs gate 25 → ADOPT THE CORRECTED DEFINITION; rim is
  SELECTIVE, not mandatory.** The gate measures the *body* read; the 1px dark
  outline is a constant frame and must not count against the delta. Rulebook
  R-LUMA is redefined as **body pixels excluding the `#1a1214` outline** (Ravager
  Δ42.7, rat Δ50.0 — both pass). The rim-light step (`bhpix.rim_light()`) is kept
  as a **selective** tool per R4 — heroes/elites/bosses/night-lit actors only,
  ≤1px bone on the lit (NW) edge, **never on common mobs** ("rim on everything"
  is an era-rubric fail). Player base bodies MAY take the faint NW rim for night
  readability (they are camera-center heroes); it must never read as a glow.
- **D2 — Player cell → 32×48 (keep).** The 32×48 cell already **passed the
  crowd15 silhouette test** (the crowd failure was name-tag pile-up, not size —
  see R-TEXT-2). 32×56 would cost T-ART-10 (`anchorY`) **plus** rewiring the
  hard-coded name (y−52) / HP (y−46) offsets in `game.cpp`, and adds a second
  anchor law. Not worth it at MVP. **Revisit trigger:** if the first in-client
  15-pile shows players getting lost, bump to 32×56 then — it is a contained
  change (anchorY + two constants). Director may override to 32×56 for the more
  imposing HB read from day one.
- **D3 — Terrain floor → keep ≈51 (floor +0).** Vessalia is a drowned, cursed
  kingdom; the dark floor is the fiction. The L1 grade is carried by sprite
  brightness (75+) and value structure, not a bright floor. **+30 is rejected**
  (collapses Ravager Δ to +13 — breaks R-LUMA). +15 is unnecessary. Consequence:
  terrain ramps can be authored now, no plate rework.
- **D4 — Callout font → 7×11 (cap 11).** Callouts are the combat-storytelling
  channel and must be legible-first; the 11px cap reads clearly better at night
  under the overlay (they sit beneath it, `game.cpp:1298–1303`) and is the more
  HB-authentic chunky read. R7's concurrency cap (≤3, 0.8s) already bounds pile
  noise. The 5×7 draft is retained if 11px proves too loud in the real pile.
  **Name tags stay small** (separate from callouts; see R-TEXT-2).

---

## Part 3 — D5–D12 rulings

| # | Ruling | Reason |
|---|---|---|
| **D5** | **Elite cell = 40×60** (true 1.25×, anchorY 52) | 48×64 is 1.5× wide and reads as a different creature class, breaking the "elite = same family, bigger + trim" rule. Registry already lists 40×60 as spec. Sibling amends the T-ART-10 card text (48×64 → 40×60). |
| **D6** | **Open T-ART-12** (textured-ground renderer) | Verified gap: ground is flat `terrainColor` diamonds (`game.cpp:679`); B1/B2 terrain is invisible in-client even with the gate open. Card scope: textured diamond draw (plate cut by world coords, `cut_diamond`), prism skins, edge lookup by adjacency. A11 manifest + A17 registry define the lookup keys. |
| **D6b** | **Amend T-ART-03**: pools draw **after** the night overlay (additive) | Verified: pools under the normal-blend overlay read mauve at 02:00. Phase-1 "0 engine change" cannot meet its own "warm pools visible at night" acceptance. Merge phase 2 into the card now; art still ships the decals (2 flicker frames, 5 colours). |
| **D7** | **Bake ×3 for MVP** — do NOT block B6 on the palette-swap shader | The shader is unscheduled engine work. 18 baked sheets (3 classes × 2 sexes × 3 tones) is tiny and ships now; adopt the A3 index maps later if the shader lands. |
| **D8** | **Open T-ART-14 (VFX strip loader) + T-ART-15 (icon draw path)** | Verified: `loadAtlas` rejects `dirs:1` (`atlas.cpp:29`); no icon/hotbar draw; no decal layer. Stopgap endorsed: A2 can emit 8-identical-row packing for VFX strips today (no engine change). T-ART-09 already covers decals. |
| **D9** | **Rename the mob, keep the class name.** Mob 1007 → **"Waxen Celebrant"** | The player class "Gravecaller" is established (`kits.h`, T-053/054). The mob's wax-mask/censer priest reads fine as "Waxen Celebrant" and the collision disappears. Registry/folders key by **id**, so only `name` in `mobs.h` changes (content owner edits; director may pick a different name — the ruling is *mob renames, class keeps its name*). |
| **D10** | **Acknowledged — no action** | Maps are generator output; humans author real maps later (ADR-007). The manifest is a re-runnable script; per-material plates/edges are unaffected by map churn. |
| **D11** | **Acknowledged — confirmed** | `docs/prompts/asset-research-bible.md` is director-owned, read-only for the art lane. The two A25 residues were purged (verified: no `8b5cf6` in `50-vfx.md` / `make_style_tile.py`). |
| **D12** | **Approve the bleed convention**: organic-over-built; WALL never bleeds | Era-correct (hand-painted iso reads as grass creeping onto path, mud onto grass, water lapping dirt). Rule: GRASS→PATH, MUD→GRASS, WATER→DIRT, BONEPIT→FLOOR, CANDLE-wax→FLOOR; WALL gets a prism footing skirt instead. The 22-row table is generated from the A11 adjacency pairs under this rule → the lookup key T-ART-12 codes against. |

**R-TEXT-2 (from the crowd test) — APPROVED, adopt into the rulebook.** The
crowd15 failure is **name-tag pile-up (7 overlaps), not silhouettes**. Rule:
when >3 overhead name tags would overlap, they degrade to the **karma-badge
glyph only**. Era-consistent (HB shows names on hover/target; L1 always-on but
shorter) and preserves the PvP karma read. Client-side render change (sibling
lane to implement).

---

## Part 4 — Engine cards for the sibling to open (proposals, not written to `docs/tasks/` by me)

- **T-ART-12** — textured-ground renderer (D6). **Unblocks B1/B2 visibility.**
- **T-ART-13** — bitmap-font path for callouts + names (D4). Ships `callout_font.png/.fnt`.
- **T-ART-14** — VFX `dirs:1` strip loader (D8). Stopgap: 8-identical-row packing.
- **T-ART-15** — icon/hotbar draw path (D8). Unblocks B7.
- **Amend T-ART-03** — pools draw after the night overlay (D6b).
- **Amend T-ART-10** — elite cell 48×64 → 40×60 (D5).

## Part 5 — Content change (content owner)

- `mobs.h`: mob 1007 display name → "Waxen Celebrant" (D9). Id/folder unchanged.

## Part 6 — What this unlocks, and the sequencing note

The B0 gate is **open**. Production may start. **Sequencing recommendation
(endorsed from the roadmap):** pull **B3 mobs ahead of B1 terrain** if the
sibling lands T-ART-01/05 first — mobs become visible with two small cards,
while terrain needs the whole T-ART-12 renderer. B1 terrain art can still be
*generated* in parallel; it just can't be *judged in-client* until T-ART-12.

Every drop goes through the offline harness (`bh_qa_sheet.py`) → triptych +
`audit.json` in `docs/research-notes/qa/`, LICENSES row per batch. Nothing is
"shipped" until it loads in-engine (T-ART-01/04/05) and passes the B9 sweep.

---

**Signed:** B0 style-lock APPROVED; D1–D12 ruled as above.
**Director override:** any single ruling can be flipped by editing this file;
the "if X → Y" consequences are in `docs/art/PARALLEL-ROADMAP.md` §5.
