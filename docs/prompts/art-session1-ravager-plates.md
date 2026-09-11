# Art Production Prompt — Ravager SESSION 1 executed, Sessions 2–3 queued (B6)

*STATUS 2026-09-10 (supersedes the "not generated" state below): Sessions 2–3
were found on disk as byte-duplicate placeholders with false acceptances —
REJECTED, quarantined (/tmp/opencode/rejected-plates/ + MANIFEST.txt), Runs
acceptances struck. Then REGENERATED keyed (flux via gen.pollinations.ai,
seeds 1001–1020, hardened pixel/green-screen prompt): 20/20 landed, all
md5-distinct, 512×768, spot-checked (m_walk_S, f_walk_S, m_die_S — walk poses,
axe right, green bg + painted shadows → hand-fix). Runs lines carry the real
seeds. Image-budget note: first weak prompt failed gate (painterly, brown bg,
idle pose); hardened prompt passed. Python-urllib UA gets CF-1010 — use a
curl-compatible User-Agent. Key used env-only, never written to disk.*

*Follows `docs/prompts/art-b1-b8-execution.md` (B0 gate OPEN, lane: `assets/aigen/`,
`docs/art/`, `docs/research-notes/`, `tools/atlaspack/`, `assets/LICENSES.md`).
Work happens in the art checkout at `/home/user/BloodHollow` (a clone of this
same GitHub repo). SESSION 1 (10 raw plates, image limit exhausted) is DONE and
UNCOMMITTED there; this prompt records it exactly and queues Sessions 2–3
(10 plates each) for a fresh turn.*

## Ground state (SESSION 1, as reported — verify on entry, do not re-derive)

- Checkout: `/home/user/BloodHollow`, cloned from the GitHub repo.
- SESSION 1 generated **10 Ravager raw plates** (4×, `_raw.png` suffix):
  - `assets/aigen/players/ravager/m/plates/ravager_m_S_4x_raw.png`
  - `assets/aigen/players/ravager/m/plates/ravager_m_SE_4x_raw.png`
  - `assets/aigen/players/ravager/m/plates/ravager_m_E_4x_raw.png`
  - `assets/aigen/players/ravager/f/plates/ravager_f_S_4x_raw.png`
  - `assets/aigen/players/ravager/f/plates/ravager_f_SE_4x_raw.png`
  - `assets/aigen/players/ravager/f/plates/ravager_f_E_4x_raw.png`
  - `assets/aigen/players/ravager/m/plates/ravager_m_attack_f1_S_4x_raw.png`
  - `assets/aigen/players/ravager/f/plates/ravager_f_attack_f1_S_4x_raw.png`
  - `assets/aigen/players/ravager/m/plates/ravager_m_cast_f2_S_4x_raw.png`
  - `assets/aigen/players/ravager/f/plates/ravager_f_cast_f2_S_4x_raw.png`
- Post step applied: background pixels normalized to exact `#00FF00`
  (chroma key; pipeline twin is `bhpix.key_out_green` — use the tool for
  Sessions 2–3, keep the manual result for Session 1 and note any delta).
- `## Runs` entries appended to `assets/aigen/players/ravager/m/prompt.md`
  and `assets/aigen/players/ravager/f/prompt.md` (one line per generation:
  date · model or "undisclosed" · seed if known · which dirs · accepted y/n).
- Deliberately NOT done: no sheets built, no QA rebuilds claimed, no
  `bh_qa_sheet.py` passes claimed.
- Turn image budget: **10/10 spent** — Sessions 2 and 3 were not generated.
- Working tree in that checkout (leave for the art committer):
  `M assets/aigen/players/ravager/f/prompt.md`,
  `M assets/aigen/players/ravager/m/prompt.md`,
  `?? assets/aigen/players/ravager/f/plates/`,
  `?? assets/aigen/players/ravager/m/plates/`
- Review in progress: `ravager_m_S_4x_raw.png` opened for review, verdict pending.

## Read first (in this order — read-only unless your lane says otherwise)

1. `docs/art/B0-GATE-DECISION.md` (binding rulings: D1 rim, D2 32×48 cell, D5 elite 40×60, D7 bake ×3).
2. `assets/aigen/players/ravager/m/prompt.md` + `f/prompt.md` — PREFIX/SUBJECT/PALETTE/NEGATIVE (frozen for all Ravager sessions; never re-word mid-class), `## Expansion` (keyframe plan), `## Runs` (SESSION 1 manifest — the sequence continues from its last line).
3. `assets/aigen/players/ravager/m/BRIEF.md` (+ f) — delivery checklist (plates → `bhpix` chain → hand-fix → `pack_atlas` → QA triptych → R-LUMA → LICENSES).
4. `assets/aigen/REGISTRY.md` + `assets/aigen/palettes/` (≤32 colours, family ramps).
5. `docs/research-notes/helbreath/readability-rulebook.md` (R-LUMA ≥25 excl. `#1a1214` outline, R-SCALE 43px body, R-TEXT-2).

## Hard rules (bind — B1–B8 §1 + lane law)

- Write ONLY: `assets/aigen/`, `docs/art/`, `docs/research-notes/`, `tools/atlaspack/`, `assets/LICENSES.md`. Never touch `engine/`, `client/`, `server/`, `shared/`, `tools/bots/`, `tools/mapgen/`, `tools/*.sh`, `data/`, `docs/tasks/`, `docs/devlog/`, `logs/`, `assets/final/`.
- B6 cell law: **32×48, body ≤ 43 px** (rows 0–42), `fit_to_cell(43)`, feet on the anchor row, anchorY 42. Ravager shoulders **14 px** measured on the S frame after downscale (hand-adjust, never re-prompt).
- Native dirs S/SE/E only for AI generation; SW/W/NW/N/NE come later via img2img (≤0.35 denoise), never mirroring (weapon stays RIGHT hand in all 8 dirs).
- Keyframes by AI: walk f0, attack f1, cast f2, die f3. Everything else (walk f1–f5, attack f0/f2, cast f0/f1/f3, hurt2, die rest, gib3) is hand-inbetweened — never spend image budget on them.
- Pale skin tone only; sallow/weathered are index swaps of the 4 skin entries. No accent hue (violet/arterial/choir-gold) on skin/cloth/gear. Kill non-source pixels above luma 200. Curse violet never `#8B5CF6`.
- Background stays exact `#00FF00` (verify per plate; prefer `bhpix.key_out_green` over manual passes from here on).
- Every output stays tagged `UNVALIDATED in engine` (sheets need T-ART-01/04/05 to display). Never claim a build pass, never fabricate a LICENSES row or QA pass.

## Step 0 — Close the SESSION 1 review (before generating anything)

1. Open `ravager_m_S_4x_raw.png` (already open) + spot-check the other 9 at 100%.
2. Run the prompt.md hand-fix list in order: outline gaps (claws/teeth/weapon tip) → right-hand weapon → feet on anchor row, no painted shadows → luma-200 kill → accent-hue sweep → shoulder width 14 px post-downscale.
3. Record the verdict as a `## Runs` follow-up line per file (accepted y/n + which hand-fix items fired). A rejected plate is REGENERATED inside a later session's budget, never silently kept.
4. Confirm the 10-file manifest above matches disk (`ls` both `plates/` dirs). Any missing/misnamed file stops the turn — fix naming first (`<class>_<sex>_<dir|action>[_<frame>]_4x_raw.png`).

## SESSION 2 — next 10 (fresh image budget required; do not start at 0 remaining)

Generate exactly these (same frozen prompts/seeds discipline as SESSION 1, `#00FF00` bg, `## Runs` line each):

1. `ravager_m_walk_f0_S_4x_raw.png`
2. `ravager_m_walk_f0_SE_4x_raw.png`
3. `ravager_m_walk_f0_E_4x_raw.png`
4. `ravager_f_walk_f0_S_4x_raw.png`
5. `ravager_f_walk_f0_SE_4x_raw.png`
6. `ravager_f_walk_f0_E_4x_raw.png`
7. `ravager_m_die_f3_S_4x_raw.png`
8. `ravager_f_die_f3_S_4x_raw.png`
9. `ravager_m_attack_f1_SE_4x_raw.png`
10. `ravager_f_attack_f1_SE_4x_raw.png`

(Rationale: completes walk-f0 contact frames both sexes/three dirs + die-f3 S anchor + attack-f1 SE. Contact frames f0/f3 gate the walk cycle; attack-SE extends the started attack set.)

## SESSION 3 — final 10 AI keyframes (fresh image budget required)

1. `ravager_m_attack_f1_E_4x_raw.png`
2. `ravager_f_attack_f1_E_4x_raw.png`
3. `ravager_m_cast_f2_SE_4x_raw.png`
4. `ravager_f_cast_f2_SE_4x_raw.png`
5. `ravager_m_cast_f2_E_4x_raw.png`
6. `ravager_f_cast_f2_E_4x_raw.png`
7. `ravager_m_die_f3_SE_4x_raw.png`
8. `ravager_f_die_f3_SE_4x_raw.png`
9. `ravager_m_die_f3_E_4x_raw.png`
10. `ravager_f_die_f3_E_4x_raw.png`

After SESSION 3 the AI-native set is COMPLETE — 30 plates (base 6 + walk-f0 6 + attack-f1 6 + cast-f2 6 + die-f3 6, m/f across S/SE/E; Sessions 1–3 cover 10 + 10 + 10). Verify the count closes at 30 before moving on.)

## After the keyframes (NOT this turn — plan only, do not start)

- Derived dirs SW/W/NW/N/NE via img2img (≤0.35 denoise) off the native plates.
- Hand-inbetweened frames (walk f1–f5, attack f0/f2, cast f0/f1/f3, hurt2, die remainder, gib3).
- `bhpix` chain (`key_out_green` → `harden_alpha` → `fit_to_cell` NEAREST → family `quantize` Bayer-2 → `outline`) → `pack_atlas` → `sheet.png` + `sheet.json` → `validate_atlas`.
- `bh_qa_sheet.py` gate + triptych (day / engine-night 02:00 / greyscale) + `audit.json` → `docs/research-notes/qa/` → LICENSES rows (model/date/post).
- Then Cultist + Gravecaller classes, same session pattern (30 AI keyframes each).

## Verify on entry (every turn, before any generation)

```
git status --short          # expect the 2 prompt.md M + 2 plates/ ?? (SESSION 1 uncommitted)
git log --oneline -3        # note the base; rebase the art branch before push
ls assets/aigen/players/ravager/m/plates/ assets/aigen/players/ravager/f/plates/
tail -n 5 assets/aigen/players/ravager/m/prompt.md   # Runs log continuity
tail -n 5 assets/aigen/players/ravager/f/prompt.md
```

## Definition of done (per session)

- Exact manifest on disk, names exact, bg exact `#00FF00`, `## Runs` line per plate (date · model/undisclosed · seed · dirs · accepted y/n), review verdicts recorded, no sheets/QA claimed, hand-off note written (tree location + suggested commit message, HANDOVER-B0.5 style). Image budget respected: 10 per turn, sessions never split across a limit wall.
