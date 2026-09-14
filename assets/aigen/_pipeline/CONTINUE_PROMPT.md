# CONTINUE_PROMPT — paste this to open the next art session

*Condensed session-starter, 2026-09-14. The binding master is
`docs/prompts/continue-artwork-2026-09-14.md`; the frozen per-class prompts
are `assets/aigen/players/<class>/<sex>/prompt.md`. This file is a pointer +
the exact batch recipe — when in doubt, the master and the frozen prompts win.*

---

You are continuing the BloodHollow AI art lane (repo checkout at
`/home/user/BloodHollow`, branch `arena/01a09f4c-bloodhollow`, open PR **#28**
— push to the same branch, never open a new PR, never touch `master`).
Image budget: **10 generations this session**. Write ONLY: `assets/aigen/`,
`docs/art/`, `docs/research-notes/`, `tools/atlaspack/`, `assets/LICENSES.md`,
`docs/prompts/`. Every asset stays tagged **UNVALIDATED in engine**.

## Verify on entry (stop and fix if any check fails)

```
git log --oneline -2        # expect dd337e0 (batch 2) at/near HEAD
gh pr view 28 --json state  # expect OPEN
ls assets/aigen/players/gravecaller/{m,f}/plates/ | wc -l   # expect 9 + 9
```

## State

- DONE: P0 mob SE natives (1004/1005, b3_build exit 0); Gravecaller 18/20 AI
  keyframes (walk_f0 S/SE/E, die_f3 S/SE/E, attack_f1 SE/E, cast_f2 SE, both
  sexes). QA montages: `docs/research-notes/qa/turn2026-09-14_batch{1,2}_montage.png`.
- This session closes Gravecaller (2 plates) and starts Cultist (8 plates).

## Generate exactly these 10 (hardened recipe below, green `#00FF00` bg)

1. `players/gravecaller/m/plates/gravecaller_m_cast_f2_E_4x_raw.png`
2. `players/gravecaller/f/plates/gravecaller_f_cast_f2_E_4x_raw.png`
3. `players/cultist/m/plates/cultist_m_walk_f0_S_4x_raw.png`
4. `players/cultist/m/plates/cultist_m_walk_f0_SE_4x_raw.png`
5. `players/cultist/m/plates/cultist_m_walk_f0_E_4x_raw.png`
6. `players/cultist/f/plates/cultist_f_walk_f0_S_4x_raw.png`
7. `players/cultist/f/plates/cultist_f_walk_f0_SE_4x_raw.png`
8. `players/cultist/f/plates/cultist_f_walk_f0_E_4x_raw.png`
9. `players/cultist/m/plates/cultist_m_die_f3_S_4x_raw.png`
10. `players/cultist/f/plates/cultist_f_die_f3_S_4x_raw.png`

## Prompt recipe (Ravager-regen pattern — never re-word the frozen core)

`PREFIX` = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look,
dark horror dark-fantasy, muddy desaturated earth palette, hard 1px dark
outline, visible dither shading, no anti-aliasing, no gradients, matte dark
blood, orthographic 2:1 isometric view" · then the frozen SUBJECT · then pose
line · then facing line · then palette + "Avoid:" block · then the wrapper:
"Nothing but the single character centered on a flat pure #00FF00 green
background, no painted shadows, no ground, crisp square pixels, entire figure
visible inside the frame."

- **Gravecaller cast_f2 E (plates 1–2)** — frozen SUBJECT from
  `players/gravecaller/<sex>/prompt.md` + "spell release frame: bronze bell
  censer swung raised aloft in the LEFT hand (raised above head height so it
  reads in profile), tiny ember glow at the censer mouth, bone knife held low
  in the RIGHT hand" + "strict side profile facing EAST, casting toward the
  right of the frame". Palette: ichor robe #1e1a20 / #2e2a30, wax half-mask
  #d9cdb4, bronze censer #6a5a3a, ember #c8622a, bone knife #cfc6b4.
- **Cultist walk_f0 (plates 3–8)** — frozen SUBJECT from
  `players/cultist/<sex>/prompt.md` (choir zealot, bone-white robe, staff-crook
  in the RIGHT hand) + "walking mid-stride contact pose, left leg forward" +
  facing line: S = "facing the viewer front-on (SOUTH)"; SE = "3/4 view facing
  SOUTH-EAST toward the lower right of the frame"; E = "strict side profile
  facing EAST, walking toward the right of the frame". Palette: choir robe
  #e6e0d4 → hem #a9a29a, hymnal sigil #4a4e58, candle flames #f2c88a (2 px
  each), crook wood #5a4630.
- **Cultist die_f3 S (plates 9–10)** — same SUBJECT + "final death frame:
  collapsed and crumpled face-down on the ground, robe pooled around the body,
  wooden staff-crook fallen and rolled beside the RIGHT hand, seen from
  slightly above, facing the viewer (SOUTH), full body low and wide in the
  frame" + add "blood pools" to Avoid.

## Acceptance gates (per plate, before any Runs line says accepted)

- md5-distinct vs every other plate (a dup = REJECTED, regenerate next session)
- corners ≈ (0–13, 247–252, 1–21); green-dominance ratio high (key intact)
- visual check on a labelled contact montage (save to
  `docs/research-notes/qa/turn<date>_batchN_montage.png` and READ it;
  head-zoom crop for anything ambiguous)
- Gravecaller: knife RIGHT / censer LEFT · Cultist: crook RIGHT hand; pose
  matches anim; no painted shadows; facing matches filename
- Rejected plate → do NOT keep silently; note it and regenerate next session

## Bookkeeping duties (sequential — never two edits to the same file in one
parallel batch; that race lost batch-1 writes in 73de497)

1. `## Runs` line per generation in each touched
   `players/<class>/<sex>/prompt.md` + status line refresh (Gravecaller →
   20/20 COMPLETE; Cultist → 8/20).
2. One `assets/LICENSES.md` row for the batch (model "undisclosed",
   provider-abstracted).
3. Update `docs/prompts/continue-artwork-2026-09-14.md` DONE/queue + this
   file's State/next-batch block (next session: Cultist die_f3 SE/E +
   attack_f1 SE/E + cast_f2 SE = 10).
4. Commit (one commit, descriptive message), `git push origin
   arena/01a09f4c-bloodhollow` → PR #28 updates itself. Report the montage.

## Next queue after this session (master has the full P1–P7 ladder)

Cultist 12 remaining → Ravager-style dir derivation (img2img ≤ 0.35, no
mirrors) + hand inbetweening + player sheet packing (no `bh_player_sheet.py`
exists yet; `atlas.draft.json` is the contract) → P2 NPCs → P3 icons →
P4 VFX → P5–P7.
