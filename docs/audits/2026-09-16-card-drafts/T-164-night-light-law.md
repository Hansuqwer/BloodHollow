# T-164 — Night light law: decide, implement, screenshot (P1, audit 15/20)

## Context
Audit findings **B9.3 / E-lane**. The spec contradicts itself and the tree:
GDD §9 asks for darkness with light radius "base 0; torches 6, lantern 8,
of-the-Vigil extend" and then records "**shipped 0** per T-076/B"; the engine has
a `lightmask.h` header and a `light` field on the wire (`EntitySpawn`/`EntityDelta`),
torches burn out and the lantern toggles (items 3003/3004), affix 8 (of the Vigil)
claims +2 radius — but there is **no additive light mask** (T-ART-03 phase 2 was
merged into the card by B0 ruling D6b and never landed), and `docs/art/00-VERIFY.md`
#7 confirms the night pass is a normal-blend fullscreen rect peaking at α150 with
world-space floaters tinted underneath it. Nobody has verified what a human
actually sees at 04:00 holding a torch.

## Scope
- **Measure first**: screenshots of Thornwall square + Bonehowl Mine at 00:00 /
  02:00 / 04:00 with (a) no light, (b) torch, (c) lantern, (d) Vigil gear, at zoom
  1 and 2. Attach to the card before choosing.
- **Then implement the D6b ruling**: additive light-pool layer composited *after*
  the night overlay (torch/lantern/anvil coals/candle clusters/Vigil), 1–2 tile
  pools, always under entities, never over the HUD; radius law from one place
  (content constants, not scattered literals).
- Decide and document the base radius (0 = pitch black outside pools, or a floor
  so the game stays playable) — this is a director call; write it into GDD §9.
- Fix the tinted-floaters issue if cheap (callouts above the overlay) or record
  the accepted behaviour.
- OUT: dynamic shadows, per-pixel lighting, day/night length changes.

## Acceptance criteria
1. Before/after screenshot matrix (3 hours × 4 light states × 2 zooms) in the PR.
2. Radius law in one constant table with unit pins (torch 6 / lantern 8 / Vigil +2
   or whatever is decided) and a wire pin that `light` matches the equipped state.
3. Night remains playable: a connected client can navigate Thornwall → fields at
   04:00 without a light source and not lose the character (director sign-off).
4. Torch burn-out + lantern toggle still work (existing pins green); no fps
   regression at 20 bots (paste fps).
5. GDD §9 amended to the decided law (T-158 cross-ref).

## Tests required
Radius-law unit pins; lightmask composition pin if the math can be made
raylib-free (headless coverage per T-154); screenshot set.

## Evidence owed at merge
Screenshot matrix, suite count, fps evidence, GDD diff, devlog, board row.
