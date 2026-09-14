# Art Production Continuation — 2026-09-14 (P0 closed, P1 queued)

*Master handoff for the image-generation lane. Read top to bottom before
generating anything. Budget: **10 images per turn** — plan batches to land on
that boundary. Lane law (binds every turn): write ONLY `assets/aigen/`,
`docs/art/`, `docs/research-notes/`, `tools/atlaspack/`, `assets/LICENSES.md`,
`docs/prompts/`. Never touch `engine/`, `client/`, `server/`, `shared/`,
`tools/bots/`, `tools/mapgen/`, `data/`, `docs/tasks/`, `docs/devlog/`,
`logs/`, `assets/final/`.*

## Verify on entry (every turn, before any generation)

```
git log --oneline -2        # expect dd337e0 (batch 2) at/near HEAD
gh pr view 28 --json state  # expect OPEN (all 2026-09-14 art commits land here)
ls assets/aigen/mobs/1004_plague_bat/plates/   # expect S, SE, E
ls assets/aigen/players/gravecaller/{m,f}/plates/ | wc -l   # 9 + 9 today; 10 + 10 after the next batch
```
python3 - <<'EOF'           # md5-distinctness spot check on any new batch
from PIL import Image; import glob, hashlib
for p in sorted(glob.glob('assets/aigen/players/*/*/plates/*.png')):
    print(hashlib.md5(Image.open(p).convert('RGB').tobytes()).hexdigest()[:8], p)
EOF
```

## DONE (sessions of 2026-09-14 — commits 73de497 + dd337e0, all in PR #28; condensed session-starter: `assets/aigen/_pipeline/CONTINUE_PROMPT.md`)

- **P0 CLOSED.** Native SE plates generated, accepted, and shipped into the
  sheets for `1004_plague_bat` + `1005_bonepicker_gnoll`
  (`sh tools/atlaspack/b3_build.sh` → exit 0, 2026-09-14; SE provenance in
  `derivation.json` = native; 1001–1003 rebuilt byte-identical — the chain is
  deterministic, don't "fix" diffs on unchanged mobs).
- **B6 Gravecaller batches 1–2 (18/20 plates)** — `players/gravecaller/{m,f}/plates/`:
  walk_f0 S/SE/E + die_f3 S/SE/E + attack_f1 SE/E + cast_f2 SE per sex (batch 2
  adds die_f3 SE/E, attack_f1 SE/E, cast_f2 SE; 1774×887, keyed `#00FF00`,
  md5-distinct, knife RIGHT / censer LEFT verified on all 18). Contact QA:
  `docs/research-notes/qa/turn2026-09-14_batch{1,2}_montage.png`.
- Bookkeeping: §Runs updated in all four touched `prompt.md` files;
  `assets/LICENSES.md` rows added. All plates/sheets remain
  **UNVALIDATED in engine** (T-ART-01/04/05 pending) — never claim otherwise.
- Process rule learned the hard way: **never issue two `edit_file` calls to
  the same file in one parallel batch** — one write per file was silently lost
  in commit 73de497 (m Runs / f status) and had to be re-recorded.

## TODO queue (strict order; one 10-image batch per turn)

### P1 — Players B6 (image lane)
Per-sex AI keyframe template = **10 plates** (Ravager m/f are the completed
reference set — compare counts against them):
`walk_f0 S/SE/E (3) + die_f3 S/SE/E (3) + attack_f1 SE/E (2) + cast_f2 SE/E (2)`.

1. **Next batch (10):** last 2 Gravecaller cast plates →
   `gc_{m,f}_cast_f2_E` (2) — Gravecaller then stands at 20/20 — then Cultist
   m/f walk_f0 S/SE/E + die_f3 S (8 of 20).
2. **Batch after (10):** Cultist die_f3 SE/E + attack_f1 SE/E + cast_f2 SE
   per sex (10 of 12).
3. Cultist final 2 (cast_f2 E) + first Ravager-derived/img2img work spills
   into the following turn(s) the same way.
- Prompts are FROZEN per class/sex in `players/<class>/<sex>/prompt.md`
  (PREFIX/SUBJECT/PALETTE/NEGATIVE + §Runs). Never re-word mid-class; append
  the exact final prompt line per generation under §Runs.
- Prompt hardening that made the Ravager regen + this batch pass (reuse it):
  frozen core + pose line ("walking mid-stride contact pose, left leg
  forward", "final death frame: collapsed… knife dropped") + facing line
  ("facing the viewer front-on (SOUTH)" / "3/4 view facing SOUTH-EAST" /
  "strict side profile facing EAST") + flat pure #00FF00 background, no
  painted shadows, crisp square pixels, whole figure in frame.
- Acceptance checks per plate: md5-distinct; corners ≈ (0–13, 247–252, 1–21);
  weapon RIGHT hand all dirs; no painted shadow; pose matches anim;
  accepted → §Runs line, rejected → regenerate in a later batch, never keep.
- After a class closes (20 plates): SW/W/NW/N/NE via img2img ≤ 0.35 denoise
  (NEVER mirror — weapon hand), hand-inbetween the non-key frames, then
  `bhpix` chain → `pack_atlas` → `bh_qa_sheet.py` triptych → LICENSES.
  NOTE: no `bh_player_sheet.py` exists yet — Ravager/GC/Cultist sheet packing
  is still manual (`atlas.draft.json` = the contract, 736×384, 23 cols).
  Stale-bookkeeping fix owed: `players/ravager/{m,f}/BRIEF.md` still say
  "no art generated" — refresh status lines when touching those files.

### P2 — NPCs final
Marta (wireKind 64) + Bounty Board (66) have **no sheets at all**; 67–73
overwrite the B5 procedural placeholders (`b5_npc_proxy.py`) with AI idle4×8
+ 96×96 portraits (`portrait_neutral/sneer` for Marta per REGISTRY).

### P3 — Icons B7
Items 2001–5103 (`icons/items/BRIEF.md`), skills ch1–ch8
(`icons/skills/BRIEF.md`; 2 need hand redraw), UI chrome
(`icons/ui/BRIEF.md`). 32×32 + 24×24 in one sheet; 24px cap on brights.

### P4 — VFX B8
31 strips per `docs/art/50-vfx.md` → `vfx/<group>/<nn>_<slug>/`, ship
`strip.png` AND 8-row packed twin (D8 loader note in REGISTRY).

### P5–P7
Castle terrain plates (`bh_terrain.py` chain, D6/T-ART-12) · unshipped mob
roster ×13 (`mobs/_gdd_roster_unshipped/`) · paperdoll refine states.

## Hand-fix checklist (after quantize, in this order — hand work, never re-prompt)

1. Outline gaps at claws/teeth/tail tip/weapon tip/censer chain.
2. Weapon/attachment RIGHT hand in all 8 dirs (img2img for W-side, no mirrors).
3. Feet on anchor row (players 42; body ≤ 43 px, rows 0–42, `fit_to_cell(43)`);
   no painted shadows (alpha-70 ellipse is added by the chain).
4. Kill pixels above luma 200 unless light source / eye glint / bone highlight.
5. No accent hue (violet/arterial/choir-gold) on skin/cloth/terrain gear;
   curse violet never `#8B5CF6`.
6. Shoulder width post-downscale (Ravager 14 / Cultist 11 / Gravecaller 10 px
   on the S frame) — adjust by hand.
7. Bat only: body bottom ≈ y30, shadow ellipse y45 (hover read).

## Pipeline cheat sheet

```
sh tools/atlaspack/b3_build.sh                       # mobs 1001–1005, exit 0 = all gates
python3 tools/atlaspack/bh_mob_sheet.py <id> ...     # per-mob chain (flags in _pipeline/README.md)
python3 tools/atlaspack/bh_pack_sheet.py <cells> <out> --pad-missing   # WIP only, never ship padded
python3 tools/atlaspack/bh_qa_sheet.py <png> --json <json> --kind sheet \
  --plate docs/research-notes/style-tile/plate_fields_mud_b0.png --out docs/research-notes/qa/<prefix>
```

Batch flow: plates → `bhpix` chain → cells → pack → QA triptych into
`docs/research-notes/qa/` → `assets/LICENSES.md` row → drop. One `## Runs`
line per generation (date · model or "undisclosed" · seed · dirs · accepted).
Every output stays tagged **UNVALIDATED in engine**.
