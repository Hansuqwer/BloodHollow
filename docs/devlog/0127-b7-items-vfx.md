# 0127 — B7.5 item icons + tier-0 VFX wired (new-key session)

## Item top-10 (10/10 fresh-cap batch, repo key)

flux.1-schnell 256px: 2001/2002/2003/2101/2102/2501/3001/3003/3004/gold
(seeds in `icons/items/prompt.md` §Runs). Probe discipline from the skill
batch carried over (hardened chroma line); all 10 accepted with documented
prop-base deviations, zero regens. Post: adaptive key, NEAREST 28px fit,
MEDIANCUT exactly 32, pipeline plate + family bevel, 24px + greyscale check.
Sheet `item_icons.png` + JSON keyed by itemId.

## Wiring + proof

`Game::ensureItemIcons` + `drawItemIcon` (lazy; rarity-plate fallback).
Bag rows (14px + shifted text), vendor + fence rows (14px + shifted text).
`tart15_vendor_icons.png`: F1–F6 + F12–F14 live, rest fall back cleanly.

## Tier-0 VFX (procedural, 0 gens)

4 PIL strips (`vfx/{swing_arc,mend_motes,cast_ring,firebolt_impact}/`,
dirs:1): crescent, motes, ring, spatter. The grey crescent failed its first
shot (grey-on-cobble) — fixed with the house 1px dark outline, re-shot green.
Feed `noteVfx` maps pulse kinds → strips (1/2/5 arc @victim, 9 impact
@victim, 8 motes @recipient, 10/13/14 ring @caster), capped 16, JSON
durations; `drawVfxPlays` over entities under floaters. Verified live
(pulse→play counter) + deterministic `--vfx-test` all-strip shot
(`vfx_tier0_all_z2.png`) + `--cam` pin flag. Art lane redraws all 4 + ~21.

## Key note

Director provided a new gateway key (`sk_…9L`) this session. Endpoint
reachable; auth UNPROVEN (the models list is public — the earlier "valid"
read was vacuous). It is stored nowhere (not in `.env`, not in docs beyond
this fingerprint); next batch's first generation is the real test. This
session's 10 gens ran on the repo key.

## Suite

Full build both presets warning-free, 396/396 + 387/387, replay mm=0,
validate 0/6. Render + art only (one shared-code exception: `--cam` and
`--vfx-test`/`drawVfxTest` dev visuals are render-only capture tools).
