# T-ART-B5 — procedural placeholder NPC sheets (kind 67–73)

| Field   | Value                                            |
|---------|--------------------------------------------------|
| Card    | T-ART-B5                                         |
| Date    | 2026-09-12                                       |
| Status  | DONE (PR #____)                                  |
| Target  | client-side render path for reserved furniture   |
| Epoch   | no worldHash impact (client-only assets + code)  |

## Why

The 7 NPC slots reserved in B5 (67 Bonesmith twins, 68 Confessor, 69 Cove Fence
(Sable), 70 Ashen Guard, 71 Synod Guard, 72 Pledge Registrar, 73 Castle
Steward) are spawnable on the server (`world.h` `debugSpawnFurniture(...)`) but
the client only draws them as a generic brown rectangle (or the one hard-coded
teal anvil). The AIGEN briefs/atlas.draft.jsons are in place, but AI plate
generation is gated outside this sandbox.

So we ship **procedural placeholder sheets** for all 7 now:
- correct cell sizes / anchorY / 8 dirs / idle4 frames (matching each folder's
  `atlas.draft.json`)
- valid v1 sheet.json the existing `loadAtlas` accepts (dirs==8, grid layout)
- 1px outline, ≤32 colours, Bayer-ish dither, feet on anchorY=42 (or the
  per-NPC override listed below) — all the rules from REGISTRY §"QA gates"
- distinct silhouettes + per-NPC palette swatches so players can read each NPC
  at a glance in-world

These are *placeholders*, not the final B5 painterly art. The briefs and
`prompt.md` files are NOT modified — final B5 art overwrites these sheets in a
future batch, no loader changes needed.

## Cell / anim summary (from `assets/aigen/npcs/*/atlas.draft.json`)

| kind | folder              | cell   | anchorY | anims                | portrait |
|------|---------------------|--------|---------|----------------------|----------|
| 67   | bonesmith_twins     | 64×48  | 42      | idle4                | 96×96    |
| 68   | confessor           | 32×48  | 42      | idle4                | 96×96    |
| 69   | cove_fence          | 32×48  | 42      | idle4                | 96×96    |
| 70   | guard_ashen         | 32×48  | 42      | idle4                | 96×96    |
| 71   | guard_synod         | 32×48  | 42      | idle4                | 96×96    |
| 72   | pledge_registrar    | 32×48  | 42      | idle4                | 96×96    |
| 73   | castle_steward      | 32×48  | 42      | idle4                | 96×96    |

(Anvil 65, Marta 64, bounty board 66 already have in-code placeholders that
read correctly; we leave them alone this batch.)

## Acceptance

- [x] 7 `sheet.png` + `sheet.json` placed into the existing reserved folders
      under `assets/aigen/npcs/<slug>/`, v1 schema, 8 direction rows × idle4
      columns, correct cell sizes, anchorY per draft
- [x] `palette.png` strip per sheet; ≤32 opaque colours
- [x] portrait.png (96×96) per NPC (bust placeholder, same palette)
- [x] client loader path for furniture sheets parallel to `mobSheetPaths`,
      with fallback to the existing brown-rect placeholder when no sheet ships
- [x] code compiles clean (cmake --build build/server-only)
- [x] QA gate: `tools/atlaspack/bh_qa_sheet.py --kind cell` equivalent checks
      (anchor row / 1px outline / palette size) reproduced in the generator's
      own self-check and printed
- [x] LICENSES.md row (procedural / project code, MIT)
- [x] task card, PR description, **do not self-merge**

## Deviations / notes

- These are *procedural placeholders* drawn by `tools/atlaspack/b5_npc_proxy.py`
  using Pillow (ellipses / rectangles / hard outlines / Bayer-2 dither). The
  final AI plates overwrite these PNGs when B5 art is produced; no code change
  is needed then because the folder contract + sheet.json schema match.
- Walk/attack/die anims are NOT added — these are idle-only furniture that
  never moves (wireIsFurniture returns true, so the locomotion code path does
  not request `walk`; it requests the idle frame used for standing NPCs). The
  client anim-frame fallback returns empty for "walk" on these atlases and
  the existing fallback chain already handles it (we keep the name "idle"
  consistent with draft).
- kJournalEpoch is NOT bumped (client-only asset change, no world hash impact).
