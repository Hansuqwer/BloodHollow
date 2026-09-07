# VFX — Boss telegraphs & world light (#28–#31)

**Status:** design brief only — no art generated (gated on B0 approval).
**Spec:** `docs/art/50-vfx.md` (sizes × frames @ fps, colour family, callout kind per effect).
**Contents:** Gravemother rot rings ×3 stages, bell shockwave + add flash, torch/candle/anvil light pools + flicker, wisp/moth night ambience

Rules (R-FX): hit FX ≤ 0.6 s · persistent effects are ground decals · ≤ 40 % caster coverage · reserved accent ramp only (violet #8B5CF6 curse, arterial #8E101C blood, choir gold #D9B04A holy) · no additive glow assumptions (engine has none).

## Generation prompt
```
"1999 MMORPG spell effect sprite frame, hand-drawn pixel art, [colour family] palette only, [shape from spec], hard edges, dithered falloff, no glow, no bloom, solid #00FF00 background, single frame, frame [n] of [N]"
+ NEGATIVE (bible §13)
```
Generate the peak/contact frame only; draw in/out frames by hand from it.

## Delivery checklist
- [ ] frame strips PNG (uniform grid) + JSON; sizes per spec
- [ ] ≤ 16 colours per effect (VFX are tighter than sprites); palette strip
- [ ] overlay test on the B0 style tile at day + night 02:00 (`docs/research-notes/qa/`)
- [ ] `assets/LICENSES.md` rows
