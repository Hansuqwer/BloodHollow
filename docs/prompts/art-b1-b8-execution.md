# BLOODHOLLOW — Art Production Prompt: Execute B1–B8

**Mode:** PRODUCTION (gate is OPEN — B0 style-lock APPROVED 2026-09-08, `docs/art/B0-GATE-DECISION.md`)
**Base:** `origin/master @ 7723929`
**Your lane (write here only):** `assets/aigen/`, `docs/art/`, `docs/research-notes/`, `tools/atlaspack/`, `assets/LICENSES.md`
**Never touch:** `engine/`, `client/`, `server/`, `shared/`, `tools/bots/`, `tools/mapgen/`, `tools/mapconv/`, `tools/*.sh`, `data/`, `docs/tasks/` (cards are proposals only), `docs/devlog/`, `logs/`, `docs/prompts/asset-research-bible.md` (director-owned, read-only), `assets/final/` (never).

## 0. Read first (in this order)

1. `docs/art/B0-GATE-DECISION.md` — the approved gate + all rulings (binding)
2. `docs/art/PARALLEL-ROADMAP.md` — the plan; its §5 decision table is now resolved by the decision doc
3. `docs/art/00-VERIFY.md`, `docs/art/01-FLAGS.md` — verified facts + known gaps
4. `docs/art/10-terrain.md`, `20-mobs.md`, `30-npcs-players.md`, `40-items-icons.md`, `50-vfx.md`, `60-paperdoll-layers.md`, `63-ui-chrome-options.md`
5. `assets/aigen/REGISTRY.md` — the id→cell→anchor→wireKind→palette contract (lookup truth)
6. `assets/aigen/palettes/` — the 13 sprite-family ramps (use them; ≤32 colours)
7. `docs/research-notes/helbreath/readability-rulebook.md` — R-rules incl. R-LUMA (body excl. outline ≥25), R-TEXT-2 (name-tag degrade), R-SCALE (43px body)
8. `tools/atlaspack/` — `bh_qa_sheet.py` (mandatory gate), `bh_pack_sheet.py`, `bhpix.py`, `make_edge_preview.py`
9. `docs/research-notes/style-tile/` — the approved tile + plates; the plates are your only generation source until a new plate is approved

## 1. Hard production rules (bind)

- **Pipeline gate every asset:** generate → `bh_qa_sheet.py` (cell/sheet/plate) → triptych + `audit.json` into `docs/research-notes/qa/` → LICENSES row. Exit 0 = pass; failures must be fixed, never shipped.
- **Cells/anchors (D2, D5, REGISTRY):** common mobs 32×48 / feet y=42; elite **40×60** / anchorY 52 (D5 — NOT 48×64); boss 64×64 / anchorY 58; player **32×48** (D2 — NOT 32×56). Draw origin is hard-coded `{w/2, 42}` in `game.cpp`.
- **All anims 8-dir** (`loadAtlas` rejects `dirs≠8`, `atlas.cpp:29`). No mirroring asymmetric gear. Direction order E,SE,S,SW,W,NW,N,NE.
- **≤32 colours per sheet/family**, from the named ramps. Curse violet = `#6B4A8A`/`#A884C4` (never `#8B5CF6`).
- **Night = engine overlay `(18,22,70,α≤150)`**, tested at 02:00 (α145). Pass = silhouette + R-LUMA Δ≥15 after the overlay. Never a 0.65 multiply.
- **R-LUMA gate:** body pixels **excluding** the `#1a1214` outline ≥25 vs terrain. Plate min-luma floor **≥24** (clamp in `make_ground_tiles`; the B0 plate's 22.1 is a known fix).
- **Saturated accents only for magic/curse/blood.** No neon on terrain/gear.
- **Every output tagged** `UNVALIDATED in engine` until it loads via T-ART-01/04/05. Nothing "passed"/"shipped" in-client without a build proof.
- **Never trace/rip/upscale** ancestor art. Plates in `style-tile/plates/` are the approved base.
- **Record model/date/post** in LICENSES; "undisclosed" explicitly if provider-hidden.

## 2. Sequencing (approved: B3 pulled ahead of B1)

Mobs become visible with two small cards (T-ART-01/05); terrain needs the whole T-ART-12 renderer. **Do B3 first, then B1/B2 terrain, then B5/B6/B7/B8** — but B6 depends on T-ART-01/04/05/D7 visibility, so its *art* can be generated in parallel while you wait. If a batch's visibility card hasn't landed, generate it anyway (offline QA passes) and mark it "generation done, in-engine UNVALIDATED."

### B3 — Mobs 1001–1005 (start here)

- 1001 Marsh Rat · 1002 Feral Ghoul · 1003 Hollow Hound · 1004 Plague Bat · 1005 Bonepicker Gnoll
- 32×48, walk4/attack3/die3 → 80 frames each, 8 dirs. Common budget ≤40 KB PNG-8.
- Use the existing `BRIEF.md` + `prompt.md` per mob (already in `assets/aigen/mobs/`). Fix any hand-fix list from the B0 work.
- Silhouette: 1001 low blob · 1002 tall S-curve · 1003 forward wedge · 1004 W with drop (body up 10px, shadow on diamond) · 1005 hunch + club. Verify greyscale vs the roster in `20-mobs.md`.
- **Gate:** each cell passes `bh_qa_sheet.py`; greyscale lineup vs 3 peers; palette from family ramp.

### B4 — Mobs 1006–1010 + boss

- 1006 Charnel Widow · 1007 Waxen Celebrant (D9 rename — the brief folder `1007_gravecaller` keeps its id) · 1008 Revenant Sexton · 1010 Sepulcher Elite (40×60) · 1009 Gravemother (64×64 boss).
- Boss: walk4/attack3/cast4/hurt2/die4/summon4 → 168 frames. Keep bell-dome lower 16px free of detail (boss occupancy finding). ≤120 KB.
- **Gate:** boss must not occlude a 5-player pile (boss_occupancy test); elites read as same-family bigger + trim (D5).

### B1 — Terrain (town + Fields of the Overflow)

- Per `10-terrain.md` + `MAPS_*` manifests (`assets/aigen/terrain/{town,fields}/`).
- Fix the plate floor clamp (min-luma 24) in `make_ground_tiles` first.
- **Edges (D12):** organic-over-built bleed (GRASS→PATH, MUD→GRASS, WATER→DIRT…); WALL never bleeds (prism footing skirt). 22 adjacency pairs from the manifests; ≥3 variants per pair.
- Terrain ramps from the palette families (D3: keep floor ≈51). Terrain is invisible in-client until T-ART-12 — mark UNVALIDATED, generate anyway.
- **Gate:** plate min-luma ≥24; tile passes plate QA; edges seamless (A12 geometry already audited true).

### B2 — Terrain (Bonehowl Mine + Drowned Crypt + Thornwall Crypt)

- Per manifests (`terrain/{mine,crypt}/`). Reuse crypt artwork across the two crypt maps, but separate usage manifests.
- Mine = dark industrial; Crypt = flooded nave + boss apse. Keep scatter as shadow anchors (Soma rule).### B5 — NPCs + portraits

- 8 NPCs (Marta, Board, Anvil, fence, guards, confessor, steward…) per `30-npcs-players.md`. Kinds 67–73 reserved in REGISTRY. Portrait channel + dialog frames (A19).
- **Gate:** fiction audit — sigils/faction colors per R-TEAM (Ashen Compact = SOOT/IRON band; Pale Synod = BONE band); karma tints `(235,60,50)` chaotic / `(170,200,255)` lawful / `(190,190,200)` neutral.

### B6 — Players (paper-doll, the long pole)

- 3 classes × 2 sexes × 8 dirs. 32×48, anchorY 42. Per `60-paperdoll-layers.md` (10-layer stack, 43px body — D2).
- **D7 = bake ×3** (skin/class tints as baked sheets, not shader): 18 sheets total. No palette-swap dependency.
- Rim (D1): ≤1px bone on lit NW edge **only for heroes/elites/night-lit**, never common mobs.
- **Gate:** paper-doll layer test (each slot reads through), R-LUMA ≥25.

### B7 — Items + icons + UI

- Per `40-items-icons.md`. Icon style from `63-ui-chrome-options.md` (rec B + reliquary).
- No icon/hotbar draw path yet (T-ART-15) — generate + QA offline, mark UNVALIDATED.
- **Gate:** rarity frame + "+5 glow" read; palette ≤32; no `#8B5CF6`.

### B8 — VFX + gore + callout font

- ~25 VFX + gore decals + **callout font 7×11 (D4)** — `bhfont.py` ships `callout_font.png/.fnt`; propose T-ART-13 for the client path.
- Pools draw **after** the night overlay (D6b — amend T-ART-03). Stopgap for VFX strips: 8-identical-row packing (D8) so `loadAtlas` accepts them.
- **Gate:** FX occlusion ≤40% of caster (R6); callouts legible under the night overlay; gore decals matte & dark.

### B9 (offline sweep, alongside)

- Re-run every shipped asset through `bh_qa_sheet.py`; compile triptychs + audit into `qa/`; 15-pile crowd test with the real cells.

## 3. Deliverable format per batch

For each batch deliver:

1. **Batch summary** — what shipped, what's UNVALIDATED, what's blocked and on which card.
2. **QA record** — the `bh_qa_sheet.py` triptych + `audit.json` path for each asset.
3. **LICENSES rows** — model/date/post for anything generated.
4. **Open decisions** raised (tag `⟨BLOCKED⟩` / `⟨DIRECTOR⟩` / `⟨SIBLING⟩`).
5. **Hand-off note** — uncommitted tree location + suggested commit message, exactly as in `HANDOVER-B0.5.md` style.

## 4. Never-do list

- Never write to `assets/final/`, the sibling's lanes, or `docs/tasks/` (card proposals go in your batch summary only).
- Never claim in-engine pass without a build. Never add a 9th direction or mirror asymmetric gear. Never exceed 32 colours. Never ship a `#8B5CF6`.
- Never fabricate a LICENSES entry or a QA pass.

**Definition of done:** every B1–B8 asset generated from the approved plates/ramps, passed offline QA, LICENSES-covered, silos UNVALIDATED-marked, and handed off in a reviewable batch with the engine-card proposals attached. The B0 gate is open — go produce.
