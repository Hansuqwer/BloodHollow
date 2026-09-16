# Art Pipeline Prompt — Ravager 30 keyframes: fix → pack → QA → LICENSES (B6)

*Follows `docs/prompts/art-session1-ravager-plates.md` (Sessions 2–3 regenerated
keyed, seeds 1001–1020, committed as `0f05921`). This prompt executes the
still-open remainder: the delivery checklist from `BRIEF.md`. Lane:
`assets/aigen/`, `docs/art/`, `docs/research-notes/`, `tools/atlaspack/`,
`assets/LICENSES.md` — never `engine/`, `client/`, `server/`, `shared/`,
`tools/bots/`, `tools/mapgen/`, `tools/*.sh`, `data/`, `docs/tasks/`,
`docs/devlog/`, `logs/`, `assets/final/`.*

## Ground state (verify on entry — close the 30 first)

- Committed here (`0f05921`): 20 keyed plates (walk-f0/die-f3/attack-f1/cast-f2 ×
  S/SE/E × m/f, 512×768 true PNG) + Runs lines with real seeds.
- **Missing here:** SESSION 1's 10 (base S/SE/E + attack-f1 S + cast-f2 S × m/f),
  uncommitted in the other clone. Step 0 below merges them; the 30-plate
  close-out count (6 base + 6 walk + 6 attack + 6 cast + 6 die) must read 30/30
  before Phase A touches anything.
- Superseded generator `tools/atlaspack/gen_ravager_sessions.py` produced the
  rejected dupe batch — **do not rerun it** (quarantine record:
  `/tmp/opencode/rejected-plates/` + MANIFEST.txt, not in repo).
- Tool truth (verified signatures, not guesses): `bhpix` is a **library**
  (no CLI — write a small driver), `bh_pack_sheet.py` and `bh_qa_sheet.py`
  are CLIs (see exact invocations below).

## Step 0 — Merge + manifest (stop on mismatch)

```
git pull --rebase   # both clones converge first
ls assets/aigen/players/ravager/m/plates/ | wc -l   # expect 15 per sex:
ls assets/aigen/players/ravager/f/plates/ | wc -l   # base 3 + walk 3 + attack 3 + cast 3 + die 3
md5sum assets/aigen/players/ravager/*/plates/*.png | cut -d' ' -f1 | sort -u | wc -l   # expect 30 (no dupes, ever again)
```

## Phase A — Mechanical chain (agent-executable, per plate, m then f)

Driver order per raw plate (function names from `tools/atlaspack/bhpix.py`):

1. `key_out_green(img, tol=0.55, despill=True)` — painted shadows go here if
   tolerance allows; survivors become Phase-B hand-fix items (record which).
2. `harden_alpha(img, cut=128)`.
3. `crop_to_alpha` (pad 0), then `fit_to_cell` NEAREST to ≤43px body rows.
4. Family palette FIRST (`build_palette(all_cells, n=32, reserve_outline=True)`
   over m+f together — shared strip per BRIEF), then
   `quantize(cell, palette, dither=<see note>, strength=0.12)`.
   - NOTE: BRIEF says Bayer-2; the signature default reads `bayer4`. Pass
     Bayer-2 explicitly per the brief; if the output dithers wrong, stop and
     flag (do not silently take the default).
5. `outline` (inside, OUTLINE_RGB `#1a1214`), `place_in_cell` (32×48, feet on
   anchor row), `feet_check(cell, anchor_y=42, tol=2)` must pass.
6. Assert: `count_colours ≤ 32`, no `#8B5CF6` anywhere, body rows ≤ 43.

## Phase B — Judgment hand-fix (human or image-edit model ONLY)

Checklist order per prompt.md (record accept/reject + fired items in Runs):
outline gaps (claws/teeth/weapon tip) → weapon in RIGHT hand, identical
across S/SE/E → feet on anchor, no painted shadows → luma-200 kill (non-source
only) → accent-hue sweep (skin/cloth/gear) → shoulders 14px post-downscale
(hand-adjust, never re-prompt). Painted ground shadows (flux habit, seen on
spot-checks) are always hand-fix, never tolerance-stretch.

## Phase C — Pack (agent-executable)

```
python3 tools/atlaspack/bh_pack_sheet.py <cells_dir> <out_dir> \
  --cell 32x48 --anchor-y 42 --max-colours 32 \
  --order idle,walk,attack,cast,hurt,die,gib --pad-missing
```

- Sheet target: 23 cols × 8 rows = 736×384 (prompt.md Cell/sheet).
- `--pad-missing` is EXPECTED now (only AI keyframes exist; inbetweens land later) — the sheet is a draft v0, tagged accordingly.
- `validate_atlas(sheet.png, sheet.json)` must return `[]`.
- `loadAtlas` proof is ENGINE lane: attach the sheet + a load request to the
  engine workstream as a proposal (cards are proposals only). Never claim
  in-engine pass from here.

## Phase D — QA gate (agent-executable)

```
python3 tools/atlaspack/bh_qa_sheet.py <sheet.png> --json <sheet.json> \
  --cell 32x48 --anchor-y 42 --hour 2.0 --gate 25.0 --night-gate 15.0 \
  --max-colours 32 [--plate <zone_plate> | --terrain-luma <n>]
```

- Exit 0 = pass; failures are fixed, never shipped.
- Triptych + `audit.json` → `docs/research-notes/qa/` naming:
  `<class>_<asset>_qa_3x.png` / `<class>_<asset>_audit.json` (B1 precedent).
- Gates: R-LUMA body-excl-outline ≥ 25 vs zone plate · night overlay
  02:00 silhouette + Δ ≥ 15 · feet tol 2 · hover n/a (footed) · die frames
  land on anchor.

## Phase E — Palette strip + LICENSES (agent-executable)

- `palette_strip(family_palette, out)` saved beside the sheets.
- `assets/LICENSES.md`: one row per sex — model `flux`, date `2026-09-10`,
  keyed `gen.pollinations.ai`, prompt file ref, `UNVALIDATED in engine`.
  (Provider is disclosed, not "undisclosed" — record `flux` + endpoint.)

## Never-do (bind)

- No `assets/final/`, no sibling lanes, no `docs/tasks/` writes, no ninth
  direction, no mirroring asymmetric gear, no >32 colours, no `#8B5CF6`,
  no fabricated QA/LICENSES, no in-engine claims without a build proof,
  no rerunning the rejected generator, no committing dupes (md5 gate in Step 0).

## Definition of done

30/30 plates present + distinct, Runs complete, cells fixed/packed/validated,
QA exit 0 with triptych + audit.json committed paths, palette strip saved,
LICENSES rows written, sheets tagged draft-v0 + UNVALIDATED, hand-off note
(tree location + suggested commit message, HANDOVER-B0.5 style). Derived dirs
(SW/W/NW/N/NE) and hand-inbetweened frames are explicitly NOT this prompt —
they are the next sessions.
