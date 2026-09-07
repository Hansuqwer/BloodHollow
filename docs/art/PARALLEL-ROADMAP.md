# PARALLEL-ROADMAP — what the art lane can produce while the B0 gate is open

**Brief:** B0.5 evaluation (evaluation only; does not advance the B0 gate).
**Date:** 2026-09-07 evening · **Repo state read:** `origin/master @ 78aa6c9`
(fetched live; local branch reset to it — my earlier local commit `f486d21` is
upstream as `56aca48`, tree-identical).
**Status header (updated after "Approved Execute", 2026-09-07 late):**
READY list executed; WARM items drafted as both-options; BLOCKED untouched.
Nothing committed (per brief §8 — the director commits). Write-list below.

| Item | Done | Where | Verified by |
|---|---|---|---|
| A25 residues | ✓ | `docs/art/50-vfx.md:33`, `make_style_tile.py:28`, `00-VERIFY.md` #22 | B0 tile md5 unchanged after rerun |
| A13/A23 QA harness + compositor | ✓ | `tools/atlaspack/bh_qa_sheet.py`, `bhscene.py`; `bhpix.py` + `luma_mean`, `rim_light`, seamless `diamond_mask`, `cut_diamond`, `edge_masks`, `blend_edge` | ravager cell: all gates pass offline; diamond lattice coverage exactly 1 |
| A16 decision board | ✓ PROVISIONAL | `docs/research-notes/style-tile/export/decision_board_3x.png` + `_audit.json` | viewed; numbers in §5 |
| A4 font drafts | ✓ both sizes | `export/font/callout_font_cap{7,11}.{png,json}`, `font_specimen_3x.png` (regenerate: `python3 tools/atlaspack/bhfont.py`) | viewed; cap11 `'` `.` `:` re-authored (stretch artefact), cap7 strip + both JSON byte-identical |
| A17 registry | ✓ | `assets/aigen/REGISTRY.md` | ids/names cross-checked with `mobs.h`, `items.h`, `wirekind.h` |
| A11 map manifests | ✓ | `tools/atlaspack/map_manifest.py` → `assets/aigen/terrain/<zone>/MAPS_<map>.{md,json,_preview.png}` ×5 | spawner/portal counts match the B0 audit; runtime furniture positions reproduced from `world.cpp` (Marta (32,16), Board (31,15), Anvil (32,15) — spiral tie-breaks ⟨UNVERIFIED⟩ without a server run) |
| A1 prompt packages | ✓ 39 | `assets/aigen/**/prompt.md` | — |
| A3 palette families | ✓ sprites (13 families, 21–31 colours) · terrain deferred (D3) | `assets/aigen/palettes/` | board viewed |
| A2 atlas drafts | ✓ 32 files, both variants for D2/D5 | `assets/aigen/**/atlas.draft*.json` | loader-rule replica clean on every file |
| A12 edge-mask generator | ✓ tool + PROVISIONAL preview board (no art) | `bhpix.edge_masks/edge_mask_for/blend_edge`, `tools/atlaspack/make_edge_preview.py` → `style-tile/export/edges/` | geometry re-derived from `iso.cpp:7–10` (+tx → screen SE, +ty → SW); face-row seamlessness true ×4 (`edge_masks_audit.json`); B = flat `terrainColor(0)` placeholder, no texture invented |
| A6 crowd test | ✓ template + first run | `docs/art/qa/61-crowd-test.md`, `docs/research-notes/qa/crowd15_*` | **new finding: name-tag pile-up (7 overlaps) → proposed R-TEXT-2** |
| A10 boss occupancy | ✓ | `docs/research-notes/qa/boss_occupancy_*` | from `drowned_crypt.tmj` rects |
| A8 luma re-audit | ✓ | `docs/research-notes/qa/luma-audit.json`; dossiers + rulebook amended (eyeball figures struck) | 15 frames measured |
| A18 light-pool decals | ✓ PROVISIONAL, 3 sizes × 2 flicker | `export/pools/` | **finding D6b: pools go mauve under the night overlay — see §5** |
| bh_pack_sheet.py (new) | ✓ | `tools/atlaspack/bh_pack_sheet.py` — cells dir → `sheet.png` + `sheet.json` (+anchorY), `--pad-missing` for WIP | 80-frame dry run packs to 320×384, loader replica clean, QA harness passes on it |
| A7 paper-doll master | ✓ both cells | `docs/art/60-paperdoll-layers.md` | **new finding: the B0 "46 px body" is 43 px (clipped); 32×56 also collides with name/HP offsets** |
| A9/A19 UI chrome options | ✓ both options + two modal framings, recommendation | `docs/art/63-ui-chrome-options.md` | — |
| A5, A14, A20 | BLOCKED, untouched | — | — |

**Corrections to this roadmap from execution:** (1) R-LUMA as defined in the
rulebook (body pixels *excluding* the outline) gives Ravager Δ **42.7**, rat
**50.0** — the B0 audit's 23.7 included the outline ring. D1 is therefore
less urgent than stated; the rim remains an option, not a fix. (2) D3: at
floor +30 the Ravager delta collapses to +13 (`decision_board_audit.json`), so
"brighter L1 floor" cannot be chosen without re-lifting every sprite family.
(3) D2b (32×56) has two more engine constants attached (name y−52, HP y−46
in `game.cpp:750, drawRemoteEnt`) — see `60-paperdoll-layers.md`.

> **TL;DR for the director (5-minute read):**
> - **Start tonight (READY):** (1) offline QA harness `bh_qa_sheet.py` +
>   scene compositor, (2) *decision-input renders* — Ravager rim on/off,
>   32×48 vs 32×56, three terrain floors, side by side from the existing
>   plates (no new generation), (3) `assets/aigen/REGISTRY.md` — the frozen
>   id→slug→cell→sheet contract T-ART-05/06/10 will code against.
> - **Do not do while the gate is open:** any plate generation (B1–B8), any
>   transition/scatter art, task-card writes (`docs/tasks/` is sibling lane),
>   anything in `engine/`, `client/`, `server/`, `shared/`.
> - **Answer D1–D5** in §5 (four are the style-tile README questions; D5 is a
>   new contradiction between my elite cell and the sibling's T-ART-10 card).
> - **Newly found engine gaps with no card:** textured-ground renderer (D6),
>   callout bitmap-font path (D4), VFX strip loader + icon draw path (D8),
>   palette-swap shader (D7). B1/B2/B7/B8 stay invisible in-client without them
>   even after the gate flips.

---

## 1. Sources verified for this evaluation (route wins)

| Fact | Source |
|---|---|
| Ground is drawn as flat-colour diamonds; walls as 3-colour prisms; no texture path | `client/src/game.cpp:658–686, 696`; `engine/render/iso.h:20–21` (`Color` params only); grep `DrawTexture` over `engine/`, `client/src` → only `game.cpp:729,764` (hero atlas) |
| Every entity draws from `heroAtlas_` (procedural placeholder); anims `"walk"`/`"idle"` only | `game.cpp:27, 724, 729, 760, 764` |
| Loader rules: `dirs` must be 8, `frames>0`, `ox+frames*fw ≤ tex.w`, `oy+8*fh ≤ tex.h`; unknown keys (e.g. `anchorY`) silently ignored | `engine/assets/atlas.cpp:18–40` |
| Wheel zoom 0.25 steps to 2.5, `Z` toggles 1↔2 | `engine/render/camera_rig.h:53–60` |
| Night overlay drawn after `EndMode2D`, before chat/HUD | `game.cpp:1298–1303` |
| Names + floaters use raylib default font at size 10 | `game.cpp:750–751, 904–906` |
| Skills bound to keys 1–5 only; no hotbar/icon draw; inventory is text rows | `game.cpp:242–249, 1049–1104` |
| `mapFileFor` knows maps 2,3,default | `game.cpp:821–827` |
| `.bhmap` = ground ids + zone ids + blocked; no tileset reference; generated at build (`assets/maps/*.bhmap` gitignored, `tools/mapconv/main.cpp`) | `shared/sim/bhmap.h:38–40`, `.gitignore:7` |
| 5 `.tmj` maps, layers `ground/(blockers)/zones/spawns/portals`, tileset `placeholder_terrain` (8 ids, no image); last map change `a6635e4` 2026-09-05 (bloodhollow-dev) | `data/maps-src/*.tmj`, `git log -- data/maps-src` |
| Palette-swap shader and gore decal layer are *planned*, not present | `docs/03-architecture.md:123,125`; no code hits |
| T-ART-01..11 exist as cards; art "does NOT block on them", B3–B6 invisible until 04/05 | `docs/tasks/art-backlog.md` (`78aa6c9`) |
| Bible still carries superseded text: "multiply + additive light mask" (L155–157), "≤256×384 / ≤512×512" (L208) — ledger `00-VERIFY.md` #7 and `01-FLAGS.md` F5 are the corrected truth | `docs/prompts/asset-research-bible.md` |
| Era-violet swapped to `#6B4A8A`/`#A884C4` in bible, rulebook, dark-eden, 5 VFX BRIEFs; residues remain in **my** lane: `docs/art/50-vfx.md:33` (`#8b5cf6`), `tools/atlaspack/make_style_tile.py:28` (unused constant) | `44ad327`; grep |
| Sibling commits since my base: `a2dcc6b` route v5c (`tools/bots/main.cpp`, `docs/devlog/0029-*.md`, `docs/prompts/campaign-pack-wall-analysis.md`) | `git log a262909..origin/master` |
| Sandbox has `g++` but **no raylib, no cmake** → cannot build a scratch client to prove `loadAtlas` | `which cmake`; no `raylib.h` |

**Could not do / did not do:** open the GitHub UI (used `git fetch` over HTTPS
instead — the workspace had lost `.git/config`, re-added `origin`); take
in-client captures (no build toolchain); prove engine load of any packed sheet
(⟨UNVERIFIED⟩ — only the Python replica of the loader rules passed).

**Brief labels I could not resolve in the repo:** "rule S-2", "R7 glyph grid",
"⟨P⟩ rows" do not exist in any file (grep). I mapped them as: S-2 → the
edge-tile rule in `docs/art/10-terrain.md` (8 hand-painted diamonds per
adjacency pair, no alpha overlays); R7 → rulebook R-TEXT; ⟨P⟩ → the dossier
claims that are eyeball estimates (see A8). ⟨UNVERIFIED⟩ mapping.

---

## 2. Dependency graph (true state, not the bible's order)

```
                        ┌──────────────────────────────────────────────┐
                        │  B0 GATE  (director sign-off + D1 D2 D3 D4)  │  state: WAITING (director)
                        └───────┬───────────────┬──────────────┬───────┘
      production edges          │               │              │
   ┌────────────────────────────┼───────────────┼──────────────┼──────────────────────────┐
   ▼                            ▼               ▼              ▼                          ▼
 B1 town+fields terrain    B3 mobs 1001-05   B5 NPCs       B6 players             B7 icons/UI
  needs: B0, D3            needs: B0         needs: B0     needs: B0, D1, D2      needs: B0 (loose)
  visible: T-ART-12*       visible: 01,05,   visible: 06   visible: 01,04,05,     visible: icon/hotbar
  + map hand-off (D10)     04(attack/die)    + portrait    10(if 56px), 07,       draw path* (none)
   │                         │               channel*      D7 shader or ×3 bake    │
   ▼                         ▼                              │                      ▼
 B2 mine+crypt            B4 mobs 1006-10 + boss            │                B8 VFX/gore/callouts
  needs: B1 lock, B0       needs: B3 lock, D5 (elite cell)  │                 needs: B0
  visible: T-ART-12*,08    visible: 05,10, 09 (telegraphs)  │                 visible: 04, 09,
   │                         │                              │                 strip loader*, font path*
   └───────────┬─────────────┴──────────────┬───────────────┴───────────────────────┘
               ▼                            ▼
        B9 QA sweep (in-client)  needs: 01,02,04,05,08 + every block above
        B9' QA sweep (offline)   needs: NOTHING — bh_qa_sheet.py is ours   ◄── the wait-filler

 * = engine gap with NO T-ART card yet (proposals in §5: D4 font path, D6 tileset renderer,
     D7 palette-swap shader, D8 VFX strip loader + icon draw)
```

| Node | State | Why | Unlocks it |
|---|---|---|---|
| B0 gate | **waiting** | 4 decisions in `style-tile/README.md:38–51` unanswered | director answers D1–D4 (A16 renders make them answerable in minutes) |
| B1 | blocked (prod) · blocked (visible) | style lock unsigned; D3 changes plate luma targets; **no textured-ground renderer** | B0 + D3; T-ART-12 (proposed) |
| B2 | blocked | as B1 + `mapFileFor` lacks 4/5 | B1 + T-ART-08 |
| B3 | blocked (prod) · blocked (visible) | style lock; every entity draws hero atlas | B0; T-ART-01/05 (walk/idle), 04 (attack/die) |
| B4 | blocked | as B3 + elite cell contradiction (D5) + anchorY | B3 + D5 + T-ART-10; 09 for telegraph decals |
| B5 | blocked (prod) · partially blocked (visible) | style lock; kinds 67–73 absent; no portrait channel | B0; T-ART-06; dialog UI (no card) |
| B6 | blocked | style lock + D1 (rim) + D2 (cell) | B0 + D1 + D2; T-ART-04, 10 (if 56 px), D7 |
| B7 | blocked (prod, loosely) · blocked (visible) | icon style is HB-bag, weakly coupled to the tile, but nothing draws icons | B0; icon/hotbar draw path (no card) |
| B8 | blocked | style lock; `loadAtlas` rejects `dirs:1` strips; no decal layer; `DrawText` callouts | B0; T-ART-04/09 + strip loader + font path (no cards) |
| B9 in-client | blocked | needs a build with 01/02/04/05/08 | sibling cards |
| **B9' offline** | **actionable** | pure Python over PNGs; reuses `make_style_tile.py` | nothing |

**"If I do nothing else, the first N evenings are spent waiting on…"** the
director's four answers, then T-ART-01/04/05 for anything to be visible, and
— unbudgeted by anyone yet — a textured-ground renderer before a single
terrain tile can be judged in-client. That is the space below fills.

---

## 3. Candidate space (brainstorm, then scored)

Scales: Director-free y/p/n · Engine-touched y/p/n · Sibling-overlap none /
1-path / real · Value-during-wait free/partial/full · Rework none/low/high ·
Leverage 0–2. **Class** per §6 rules (READY / WARM / BLOCKED).

| # | Candidate | Filing path (my lane) | Unblocks | Dir-free | Eng | Overlap | Value | Rework | Lev | Class |
|---|---|---|---|---|---|---|---|---|---|---|
| A13+A23 | **Offline QA harness**: `bh_qa_sheet.py` (any cell/sheet → day \| night-02:00 \| grey triptych + R-LUMA + colour count + loader-rule lint) and `bhscene.py` (compositor lifted out of `make_style_tile.py`) | `tools/atlaspack/`, output `docs/research-notes/qa/` | every batch's §15 gate; A6, A10, A16 | y | n | none | full | none | 2 | **READY** |
| A16 | **Decision-input renders** from the *existing* 4 plates: Ravager rim off/on (D1), 32×48 vs 32×56 body (D2), terrain floor at lift {current, +15, +30} (D3), callout in both cap heights (D4) — one board, labelled PROVISIONAL | `docs/research-notes/style-tile/export/` | closes B0 faster; B6, B1 | y (produces the inputs) | n | none | full | none | 2 | **READY** (both-options by construction) |
| A17 | **Asset registry**: id → folder slug → cell → sheet dims → wireKind → palette family; reserved ids (kinds 67–73, items 5001/5101–5103); `anchorY` convention text | `assets/aigen/REGISTRY.md` | T-ART-05/06/10 contract; every BRIEF | y (D5 both rows listed) | n (read-only contract) | none | full | low | 2 | **READY** |
| A11 | **Per-map art manifest script** (`map_manifest.py`, reads `.tmj` only): zone ids, spawner rects → scatter-density bands (Mir rule), portals, furniture/light-source positions, transition pairs (already measured) | `tools/atlaspack/`, `assets/aigen/terrain/<map>/MAPS.md` | B1/B2 composition; T-ART-03 pool placement | y | n | none (read-only; regenerable if maps change) | full | low (D10) | 1 | **READY** |
| A1 | **Prompt packages** per remaining asset: prefix + subject + *named palette line* + negative + S/SE/E native + 5-dir img2img notes + per-asset hand-fix list (outline gaps, weapon hand) | `assets/aigen/**/prompt.md` | B1–B8 design time | y (era-violet already swapped) | n | none | full | low | 1 | **READY** |
| A3 | **Palette family matrix**: authored ≤32 ramps for 7 mob + 3 NPC + 3 player families, skin-tone index maps, era-violet swap diagram; terrain ramps *after* D3 | `assets/aigen/palettes/` | B3–B8 consistency; LICENSES | sprites y / terrain p | n | none | full | low (sprites) · high (terrain before D3) | 2 | **READY** (sprite families) / **WARM** (terrain) |
| A2 | **Atlas JSON drafts** per sheet (offset blocks, byte budgets) — loader rules *are* confirmed (`atlas.cpp`), so labelled UNVALIDATED-in-engine only | `assets/aigen/**/atlas.draft.json` | removes JSON wait; T-ART-10 | mobs y · players/elites p (D2, D5) | n | none | partial | low (emit both cell variants) | 1 | **READY** (mobs/NPC) / **WARM** (players, elites) |
| A4 | **Callout bitmap font prototype** — 7 px cap (parity with raylib default @10 ⟨UNVERIFIED⟩ cap height) *and* 11 px cap (rulebook R-TEXT), black outline, night plate | `docs/research-notes/style-tile/export/font/` | D4, B8; plugs into `make_style_tile.py` | p (D4) | n (to *use* it needs a client font path — no card) | none | full | low | 1 | **WARM** (draft both) |
| A12 | **Edge-mask generator** in `bhpix` (4 edges + 4 corners per pair, feathered 1–2 px, iso-correct) + seam QA | `tools/atlaspack/bhpix.py` | B1/B2/castle (22 pairs × 8) | y | n | none | partial | none | 2 | **READY** |
| A6 | **15-pile crowd test** template: placements, layer order, callout load, checklist; first run with B0 cells | `docs/art/qa/61-crowd-test.md`, `docs/research-notes/qa/` | B9; rulebook R-SILH/R-FX numbers | y | n | none (in-client version marked proposal) | partial | none | 1 | **READY** |
| A7 | **Paper-doll layer master**: base vs gear slots, pivot table per dir, skin-tone mapping, shadow ownership; both 48/56 cells | `docs/art/60-paperdoll-layers.md` | B6 (long pole) | p (D2, D7) | n | none | partial | low (both-cells draft) | 1 | **WARM** |
| A10 | **Boss occupancy study**: Gravemother 64×64 + 8 elite camps + 5-player pile thumbnails from the drowned_crypt spawner rects (`gravemother_font` x21–25 y2–5, `apse_sexton`, 8 × 1010 rects) | `docs/art/62-boss-occupancy.md` | B4 composition, R-SILH boss rule | y | n | none | partial | none | 1 | **READY** |
| A8 | **Dossier claim re-audit**: the luma figures (HB floor ≈58 / sprites 110–140, L1 ≈95/150, Soma 35/140) are eyeball estimates → re-measure with ROI sampling on the archived frames; confirm or strike | `docs/research-notes/*/findings.md` (edits), `qa/luma-audit.json` | removes ⟨UNVERIFIED⟩ debt under R-LUMA | y | n | none | partial | none | 1 | **READY** |
| A9 | **UI/icon style pre-spec** two ways (iron-parchment vs silver-filigree emphasis) | `docs/art/63-ui-chrome-options.md` | B7 warm-up | p (chrome not on the decision list; GDD §2 already leans HB+L1) | n | none | partial | low | 0 | **WARM** (low priority) |
| A5 | Transition-pair *art* pre-drafts (≥3 variants per pair) | `assets/aigen/terrain/` | B1 | n (B0 style lock, D3) | n | none | — | **high** | — | **BLOCKED** (tool half → A12) |
| A25 | **Lane residue fixes**: `50-vfx.md:33` hex, `make_style_tile.py:28` constant, `00-VERIFY.md` #22 wording vs D5 | own lane | consistency | y | n | none | free | none | 0 | **READY** (10 min) |
| A14 | Scratch-client `loadAtlas` proof of a packed sheet | — | tech audit | y | p | none | — | — | — | **BLOCKED** (no raylib/cmake in sandbox; needs sibling/CI build) |
| A15 | Proposed engine cards T-ART-12..15 (text only, for the sibling to open) | `docs/art/PARALLEL-ROADMAP.md §5` | B1/B2/B7/B8 visibility | y | n (proposal) | 1-path (card *creation* is sibling's) | full | none | 2 | **READY** as proposal only — never written to `docs/tasks/` by me |
| A18 | Light-pool decal set (procedural radial dithers, T-ART-03 phase 1) | export/ | T-ART-03 | y | n | none | partial | low | 1 | **WARM** — note T-ART-03 "0 engine change" is optimistic: no decal draw path exists (needs 09 or 12) |
| A19 | Portrait/dialog frame mock | docs/art | B5 | p | n | none | partial | low | 0 | WARM (after A9) |
| A20 | Bot-driven in-client screenshot harness | `tools/bots/` | B9 | y | y | **real** | — | — | — | **BLOCKED** (sibling lane) |

---

## 4. Recommendation

### Start this evening (three READY items, in this order)

1. **A13/A23 — offline QA harness + compositor.** Everything else in this list
   renders through it; it is the B9' node that needs nothing. Inputs: any
   cell/sheet PNG (+ optional JSON); outputs: triptych PNG + `audit.json`
   (R-LUMA Δ vs a chosen plate, opaque colours ≤32, loader-rule lint,
   night alpha from `daynight` port). Rework risk zero — decisions change
   numbers, not the harness.
2. **A16 — decision-input board.** Re-run the B0 chain from the four existing
   plates (no new generation) into one PROVISIONAL board:
   D1 Ravager rim off | on · D2 body 43 px | 50 px (32×56 cell) · D3 floor
   lift 0 | +15 | +30 · D4 callout 7 px | 11 px cap (needs A4's draft glyphs
   — do A4's 7-px set first, ~40 glyphs). File under
   `docs/research-notes/style-tile/export/`, provenance row added. This does
   **not** sign the gate; it makes the four answers a five-minute job.
3. **A17 — `assets/aigen/REGISTRY.md`.** Freeze the lookup contract before the
   sibling codes T-ART-05/06/10: `1001_marsh_rat … 1010_sepulcher_elite`,
   kinds 64–66 shipped / 67–73 reserved, cells (both D5 rows shown), sheet
   dims, `anchorY` default 42, palette family, item/icon reserved ids. Slugs
   are keyed by **id** so F2 (1007 rename) cannot break paths.

### Second half of the wait (order of leverage)

A11 map-manifest script → A1 prompt packages (all 39 BRIEF folders) → A3
sprite-family palettes + skin-tone maps → A12 edge-mask generator → A2
atlas.draft.json (mobs/NPCs; players+elites emitted in both cell variants) →
A6 crowd test (with B0 cells) → A10 boss occupancy thumbnails → A8 luma
re-audit → A7 paper-doll layer master (both cells) → A25 residues → A9/A19
only if time remains.

### The moment the gate flips

- Day 0: consolidate WARM drafts to the chosen options (delete the losing
  variant of A2/A7/A16; set terrain ramps per D3; font per D4).
- B1 starts with the fields plate re-prompt ("furrows at 26.6°") and the
  22-pair edge set through A12; **but** flag loudly that nothing terrain
  renders in-client until T-ART-12 exists — recommend B3 (mobs) be pulled
  *ahead* of B1 if the sibling lands T-ART-01/05 first, because mobs become
  visible with two small cards while terrain needs a new renderer.
- Every drop goes through the harness; triptychs + audit JSON land in
  `docs/research-notes/qa/`, LICENSES rows per batch.

---

## 5. Decision-needs table

| # | Decision / gap | Owner | What is at risk | "If X → the plan does Y" |
|---|---|---|---|---|
| D1 | Ravager R-LUMA Δ 23.7 vs gate 25 (`style-tile/README.md:38`) | director | player readability on mud; B6 pipeline step | (a) accept 23 as player gate → no rim step, rulebook R-LUMA gets a player clause · (b) rim → `bhpix.rim_light()` added, all B6 bodies get the 1-px bone rim; A16 shows both |
| D2 | Player cell 32×48 vs 32×56 (`README.md:43`) | director (+ T-ART-10 for 56) | B6 sheet geometry 736×384 vs 736×448; A2/A7 drafts | 48 → no `anchorY`, T-ART-10 only for elites/boss · 56 → `anchorY:50`, T-ART-10 becomes a B6 visibility prerequisite |
| D3 | Terrain floor luma ≈51 (current) vs L1-brighter (`README.md:47`) | director | terrain palettes (A3), prompt luma line (A1), R-LUMA margins | keep → terrain ramps authored now · brighter → sprite families need +luma re-check; terrain ramps wait; no plate rework (none exist) |
| D4 | Callout font cap 7 px vs 11 px (`README.md:51`) + **no client bitmap-font path** (`game.cpp:904` uses `DrawText`) | director (style) · sibling (path) | B8 callouts; night legibility of floaters (they sit under the overlay, `game.cpp:1298–1303`) | either → A4 ships `callout_font.png/.fnt`; propose **T-ART-13** "bitmap font for floaters + names" |
| D5 | **Elite cell contradiction**: `docs/art/20-mobs.md` = 40×60 (true 1.25×) vs `docs/tasks/art-backlog.md` T-ART-10 = 48×64 | director; sibling edits the card | B4 sheet dims (480×480 vs 576×512), registry row, `anchorY` 52 vs 56 | 40×60 → registry as written, card text amended by sibling · 48×64 → I amend `20-mobs.md`/BRIEF; anchorY 56. My recommendation: 40×60 (48×64 is 1.5× wide and reads as a different creature class) |
| D6 | **No textured-ground renderer** (flat `terrainColor` diamonds, `drawPrism` colours) — no card | sibling/engine | B1/B2 invisible in-client even after the gate; T-ART-03 phase 1 has nowhere to draw | propose **T-ART-12**: textured diamond draw (plate cut by world coords, `cut_diamond` semantics), prism skins, edge lookup by adjacency; A11 manifest + A17 registry define the lookup keys |
| D6b | **Light pools die under the night overlay.** `export/pools/pools_preview_3x.png`: ember pools composited under the engine's normal-blend overlay read mauve at 02:00. T-ART-03 phase 1 (painted decals, "0 engine change") cannot meet its own acceptance ("warm pools visible at night") | sibling (T-ART-03 scope) | R-NIGHT light pools; mine/crypt readability | pools must draw **after** the overlay (additive) or the overlay needs holes → recommend merging phase 2 into the card now; art still ships the decals (2 flicker frames, 5 colours) |
| D7 | Palette-swap shader is "planned" (`03-architecture.md:123`) | engine | B6 skin tones / class tints: 1 sheet + index maps vs 3 baked sheets per (class, sex) | shader lands → A3 index maps used · not by B6 → bake ×3 (sheet count 18, still small) |
| D8 | `loadAtlas` rejects `dirs:1` (`atlas.cpp:29`); no icon/hotbar draw; no decal layer | sibling/engine | B7, B8 invisible | propose **T-ART-14** VFX strip loader (or 8-identical-row packing as a stopgap — A2 can emit that today), **T-ART-15** icon draw path; T-ART-09 already covers decals |
| D9 | Mob 1007 name collides with class (F2) | content owner | folder slug stability | registry keys by id; a rename changes `name` in `mobs.h` only |
| D10 | Maps are generator output; ADR-007 says humans author real maps later; sibling last touched maps 2026-09-05 | sibling | A11 placements, scatter composition | manifest is a script → re-run; per-material plates/edges unaffected |
| D11 | Sibling/director edited my lane in `44ad327` (bible, rulebook, dark-eden, VFX BRIEFs, LICENSES) | director | none (accepted); two residues left in my lane (A25) | I treat `docs/prompts/asset-research-bible.md` as director-owned: read-only for me from now on |
| D12 | **Edge-piece priority order** — for each of the 22 adjacency pairs (`10-terrain.md`), which material bleeds over which (GRASS over PATH or PATH over GRASS?). The A12 masks are symmetric, but the choice decides *on which tile* the piece lives → the lookup key T-ART-12 codes against | director (look) · sibling (key format) | B1/B2 edge set (176 pieces) painted on the wrong base = repaint | convention proposal: the *softer/organic* material bleeds over the *built* one (GRASS→PATH, MUD→GRASS, WATER→DIRT, BONEPIT→FLOOR, CANDLE-wax→FLOOR); WALL never bleeds (prism footing skirt instead). Answer = a 22-row table; until then `export/edges/` shows only the geometry |

---

## 6. Write-lane table

**Written in this evaluation:** `docs/art/PARALLEL-ROADMAP.md` (this file). Nothing else.

**Planned writes if the READY list is approved (all my lane):**

| Path | Item |
|---|---|
| `tools/atlaspack/bh_qa_sheet.py`, `tools/atlaspack/bhscene.py`, `tools/atlaspack/map_manifest.py`, `tools/atlaspack/bhpix.py` (+`edge_masks`, `edge_mask_for`, `rim_light`), `tools/atlaspack/make_edge_preview.py` | A13/A23, A11, A12 |
| `docs/research-notes/style-tile/export/**` | A16 board, A4 font drafts, A18 pools, A12 edge-mask preview (`edges/`, PROVISIONAL) |
| `docs/research-notes/qa/**` | triptychs, `audit.json`, `luma-audit.json` (A8) |
| `docs/research-notes/{dark-eden,lineage1,helbreath,mir2,soma}/findings.md` | A8 confirm/strike edits only |
| `assets/aigen/REGISTRY.md`, `assets/aigen/palettes/**`, `assets/aigen/**/prompt.md`, `assets/aigen/**/atlas.draft.json`, `assets/aigen/terrain/<map>/MAPS.md` | A17, A3, A1, A2, A11 |
| `docs/art/60-paperdoll-layers.md`, `docs/art/qa/61-crowd-test.md`, `docs/art/62-boss-occupancy.md`, `docs/art/63-ui-chrome-options.md`, `docs/art/50-vfx.md:33`, `docs/art/00-VERIFY.md` #22 | A7, A6, A10, A9, A25 |
| `assets/LICENSES.md` | one row per prototype in `export/` |

**No-touch list (sibling / director lane):** `engine/`, `client/`, `server/`,
`shared/`, `tools/bots/`, `tools/mapgen/`, `tools/mapconv/`, `tools/*.sh`,
`data/maps-src/`, `docs/tasks/` (T-ART-12..15 are *proposals in §5*, not
cards), `docs/devlog/`, `logs/`, `docs/01-research.md`, `docs/02-gdd.md`,
`AGENTS.md`, `docs/prompts/asset-research-bible.md` (director edits it),
`assets/final/` (never).

Collision check against `origin/master` since `a262909`: sibling files touched
= `tools/bots/main.cpp`, `docs/devlog/0029-*.md`,
`docs/prompts/campaign-pack-wall-analysis.md`, `docs/tasks/art-backlog.md`;
intersection with the planned-writes table = **∅**.

---

## 7. Provenance + QA status

| Artefact | Model / date / post | Status |
|---|---|---|
| 4 B0 plates (`style-tile/plates/*_4x_raw.png`) | Arena Agent Mode image generation, **model/version undisclosed** (provider-abstracted), 2026-09-07; post = `make_style_tile.py` (deterministic, md5-stable across runs) | research only; LICENSES row present (`44ad327` wording) |
| Style-tile composites, cells, tiles, palette strips | derived, `make_style_tile.py` | UNVALIDATED in engine (loader rules replicated in Python only; no raylib build here) |
| Reference frames (YouTube storyboards, web captures) | not generated; citation only | never shipped, never traced |
| Prototypes proposed in §4 (A16 board, A4 font, A18 pools) | **none produced in this evaluation** | when produced: `export/` + LICENSES row, labelled PROVISIONAL / UNVALIDATED |
| All `BRIEF.md`, this roadmap | text | drafts; nothing "passed" or "shipped" |

---

## 8. Definition of done for this evaluation

- [x] A reviewer can act from this page: **do** A13 → A16 → A17 this week;
      **don't** generate B1–B8 plates, transition art, or touch `docs/tasks/`
      while the gate is open.
- [x] Every fact carries a source (§1, inline paths) or ⟨UNVERIFIED⟩
      (brief label mapping; raylib default cap height; dossier luma figures;
      in-engine load of any packed sheet).
- [x] Zero writes in the sibling lane (§6; one file written, in `docs/art/`).
- [x] No volume art generated; no prototype produced; nothing committed.

### Assumptions / unverified (explicit)

1. Brief rule IDs S-2 / R7 / ⟨P⟩ are not in the repo — mapped by best guess (§1).
2. raylib default font cap height ≈7 px at size 10 — from memory, ⟨UNVERIFIED⟩;
   A4 measures it from a rendered frame before choosing the parity size.
3. Dossier luma numbers are eyeball estimates from captures — ⟨UNVERIFIED⟩ until A8.
4. `56aca48` is my B0 work re-committed by the director (tree-identical to my
   local `f486d21`, verified by `git diff`); I did not author `44ad327`/`78aa6c9`.
5. The sibling agent's *current* in-flight work is unknown beyond its commits;
   nothing here describes it from guesses.

---

## 9. Hand-off note (end of B0.5 execution)

**Uncommitted, all in lane (`git status --porcelain`: 116 entries, 165 files with `-uall`; 13 modified, rest new):** `docs/art/` (7 files), `docs/research-notes/`
(dossier amendments, `qa/`, `style-tile/export/` incl. `edges/`), `assets/aigen/` (prompt.md ×39,
atlas.draft ×32, palettes, REGISTRY, MAPS ×5), `tools/atlaspack/` (10 scripts),
`assets/LICENSES.md`. Suggested commit split, if the director wants the
one-system-one-commit discipline kept:

1. `art(tools): offline QA harness, packer, compositor, font, edge masks + preview` — `tools/atlaspack/*`, `assets/aigen/_pipeline/README.md`
2. `art(B0.5): decision board, pools, font drafts, qa runs, luma re-audit` — `docs/research-notes/**`
3. `art(briefs): registry, prompt packages, atlas drafts, palettes, map manifests, paper-doll + chrome + crowd docs` — `assets/aigen/**`, `docs/art/*`, `assets/LICENSES.md`

**Every generated image in this drop is derived from the four B0 plates or is
procedural; no new AI generation happened. Nothing is marked passed in-engine.**

**Open for the director (unchanged list, sharper numbers):** D1 (now optional),
D2 (32×56 costs two more constants), D3 (+30 floor breaks R-LUMA), D4 (font size),
D5 (elite cell), D6/D6b (textured ground; pools die under the overlay),
D12 (edge-piece priority order, 22 rows), R-TEXT-2 (name-tag degrade rule) — all in §5 and the linked audits.

**The gate is still open.** B1–B8 generation remains off until it closes.

**Late additions (same evening):** A12 preview board rendered (`export/edges/`, README there); cap11 punctuation fixed; nothing else changed. Optional leftovers: none on my side.
