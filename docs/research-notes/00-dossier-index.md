# Ancestor research dossiers — B0 (issued 2026-09-07)

Bible §3 deliverable. One page per ancestor, each answering the bible's listed
questions from **captured footage**, not memory. Every screenshot referenced
here is archived under this folder (contact sheets = evenly spaced storyboard
frames from the director-linked videos; `-tNNNNs.jpg` = 2× key frames).

| Ancestor | Lock it owns | Dossier | Primary evidence in repo |
|---|---|---|---|
| Dark Eden (1997, SOFTON) | horror register; the *only* neon licence | [`dark-eden/findings.md`](dark-eden/findings.md) | `dark-eden/darkeden-awakening-pvp-*.jpg` (director link) |
| Lineage 1 (1998, NCSoft) + Remastered | base colour grade; karma name-tint; siege crowd | [`lineage1/findings.md`](lineage1/findings.md) | `lineage1/l1-void-4v-pledge-*.jpg`, `lineage1/l1-remaster-classic-pets-*.jpg` (director links) |
| Helbreath (1999, Siementech) | characters, VFX, combat storytelling | [`helbreath/findings.md`](helbreath/findings.md) + [`helbreath/readability-rulebook.md`](helbreath/readability-rulebook.md) | `hb-video-thumb.jpg`, `helbreath/hb-koreahb-official-*.jpg`, `helbreath/hb-olympia-pestilence-ep21-*.jpg` (director links) |
| Legend of Mir 2 (2001, WeMade) | grind texture, refine theatre | [`mir2/findings.md`](mir2/findings.md) | `mir2/mir2-web-{1..8}.jpg` + contact (web captures; no director link supplied) |
| Myth of Soma (2001, Digital Bros) | terrain lock, night cap | [`soma/findings.md`](soma/findings.md) | `soma/soma-web-{1..4}.jpg` + contact; LEGOC feature page quoted |

**Style-lock proof** (bible §0 step 2): [`style-tile/README.md`](style-tile/README.md)
— 256×256 field + Marsh Rat + Ravager + Firebolt, pushed through the real
cleanup pipeline (`tools/atlaspack/bhpix.py`) at engine scale, rendered day /
engine-night / greyscale. **Awaiting director approval before any volume work.**

## Cross-ancestor findings that changed the spec (read these first)

1. **The engine's night is already under the Soma cap.** `render/daynight.cpp`
   peaks at alpha **150/255 = 59 %** (04:00) — inside the bible's ~65 % ceiling.
   The night-floor test therefore uses the *real* overlay (`bhpix.night_floor`
   ports the keyframes), not a hypothetical 65 % multiply. Note it is a
   normal-blend fullscreen rectangle drawn **after** `EndMode2D()` and before
   the HUD — so the HUD stays clean but the world-space callouts/name tags are
   tinted (hence the R-TEXT night plate); the bible's "additive light mask"
   does not exist yet (flagged in `docs/art/00-VERIFY.md`).
2. **All five ancestors put team/alignment identity in the name tag, not the
   sprite.** L1 red name, HB green party names, DE guild tag plate. The bible's
   rule (§9 "sprites do NOT change") is era-correct; `karmaBand` already ships.
3. **Every ancestor's ground is darker and lower-contrast than its sprites.**
   Measured on the HB koreahb thumbnail: dungeon floor mean luma ≈ 58, sprite
   bodies ≈ 110–140. On the L1 Void capture: stone ≈ 95, characters ≈ 150+.
   This is the single most important readability lever and it is now a
   numeric gate: **R-LUMA ≥ 25** (sprite body mean − terrain mean, 0–255) in the
   rulebook. Our first style-tile pass *failed* it (Δ 10) until the terrain
   floor was lifted and the sprite palette gamma-lifted; it now passes (Δ 24–27).
4. **Painted-plate-then-cut terrain.** Soma, Mir 2 and L1 do not tile a biome
   from 6 random diamonds — they paint a large plate and *cut* diamonds at
   screen position (Mir's map format literally stores back/middle/front image
   indices per cell). Random-crop tiling produced a visible checkerboard in the
   style tile; cutting from one plate fixed it. §6 anatomy is amended: "≥6
   non-repeating base tiles" becomes "≥1 painted 8×8-tile plate per ground
   type, cut to diamonds by `cut_diamond()`", plus edge tiles between biomes.
5. **Frame counts are honest to the era.** HB walks are ~8f, attacks 3–4f;
   L1 classic is similar; nobody tweened. Our 6/3/4/2/4/3 set is inside the
   envelope. What the ancestors *do* spend frames on is **death** (4–6f, often
   with a distinct corpse frame) — the gib 3f + petrify-fade vestige already
   in T-066 matches that priority.
