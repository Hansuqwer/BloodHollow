# Style-tile plate prompts (reproducibility record, bible §13/§16)

Generated 2026-09-07 via Arena Agent Mode image generation (model family:
diffusion, provider-abstracted; see `assets/LICENSES.md` row "B0 style-tile
plates"). All four are 1408×768 RGB plates at ~4× target, chroma-key green
`#00FF00` where a background was requested. Downstream processing is entirely
in `tools/atlaspack/make_style_tile.py` (deterministic).

## marsh_rat_4x_raw.png (mob 1001)

> 1999-era isometric MMORPG monster sprite, hand-drawn 2D pixel-art look, dark
> horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red,
> bone off-white, soot grey, stagnant green), hard 1px dark outline, visible
> dither shading, no anti-aliasing, no gradients, matte dark blood,
> orthographic 2:1 isometric view. Subject: Marsh Rat — a swollen drowned rat
> the size of a small dog, matted wet grey-brown fur clumped into spikes,
> bloated pale grey-pink belly, black ichor drooling from the mouth, naked
> grey-pink tail, low skittering posture, sickly and pathetic. Palette: mud
> brown, wet slate grey, grey-pink flesh, bone off-white teeth, black ichor,
> one dull rust-red eye. Pose: 3/4 view facing south-east toward the viewer,
> full body, all four feet on the ground plane, single creature, centered,
> small in frame with empty margin around it. Background: plain flat solid
> bright green #00FF00, no ground, no shadow, no text. Avoid: anime, chibi,
> cute, bright saturated colors, smooth gradients, 3D render, bloom, lens
> flare, text, watermark, extra limbs.

Result note: on-brief. Ichor drool + rust eye survive the 20 px downscale as
1–2 px marks; tail reads. Facing came out E/SE (usable as dir 0/1 source).

## ravager_4x_raw.png (player class, male, sallow tone)

> 1999-era isometric MMORPG player-character sprite, hand-drawn 2D pixel-art
> look in the style of late-90s Korean isometric online RPGs, dark horror
> dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone
> off-white, soot grey), hard 1px dark outline, visible dither shading, no
> anti-aliasing, no gradients, orthographic 2:1 isometric view. Subject:
> Ravager — a human male warrior with heavy shoulders, soot-grey padded
> gambeson with rusted iron plates nailed on, a butcher-chain belt with iron
> hooks, an oversized notched cleaver-axe held low in the right hand (weapon
> drawn about 15 percent larger than realistic), pale weathered scarred skin,
> close-cropped dark hair, no helmet, grim expression. Palette: soot grey, mud
> brown, rust red, bone off-white, iron blue-grey, grey-pink skin. Pose: 3/4
> view facing south toward the viewer, standing idle, weight forward, full
> body, both feet on the ground plane, single character, centered, empty
> margin around the figure. Background: plain flat solid bright green #00FF00,
> no ground, no shadow, no text. Avoid: anime, chibi, cute, bright saturated
> colors, smooth gradients, 3D render, bloom, lens flare, text, watermark,
> extra limbs, elf ears, sparkle.

Result note: on-brief; came out facing SW-ish (dir 3 source). Cleaver is held
in the *right* hand — the paper-doll rule says it must stay right-handed in all
8 dirs (no mirroring).

## fields_ground_4x_raw.png (terrain, Fields of the Overflow)

> Pre-rendered painterly ground texture in the style of Myth of Soma (2001
> isometric MMORPG), dark horror dark-fantasy, top-down orthographic view,
> seamless tileable texture. Biome: the Fields of the Overflow — drowned
> farmland, waterlogged rotten plough furrows running diagonally, black sucking
> mud, ragged patches of sickly stagnant-green grass, scattered rotten straw,
> two or three small puddles of still black water with faint painterly
> reflections, a few bone-white pebbles. Muted desaturated palette only: mud
> brown, stagnant green, soot grey, bone off-white flecks, deep black-blue
> water. Even, flat overcast lighting, no strong shadows, no vignette, no
> objects, no characters, no buildings, no text, no border. Avoid: bright
> saturated colors, neon, lens flare, bloom, 3D render, blurry photo,
> watermark.

Result note: excellent painterly read; too high-contrast raw (luma 18–140) —
the pipeline pulls contrast to 0.70 and lifts the floor. Diagonal furrows
happen to align with the iso diamond axis, which is a lucky win; request that
explicitly in B1 ("furrows at 26.6° matching a 2:1 isometric grid").

## dead_tree_4x_raw.png (scatter, fields/crypt-yard)

> Pre-rendered painterly game object in the style of Myth of Soma (2001
> isometric MMORPG), dark horror dark-fantasy, isometric 3/4 view from above,
> single object centered. Subject: a gnarled dead willow tree with a massive
> exposed root cluster gripping black mud, roots sprawling wide across the
> ground, split hollow trunk, a few bare crooked branches, rags of grey moss
> hanging, a rusted iron bell-hook nailed to the trunk. Palette: grey-brown
> bark, soot grey, black mud, bone off-white, muted stagnant green moss, tiny
> rust-red accent. Even overcast light, muted desaturated colors, painterly but
> crisp edges, full object visible including the roots at the base, empty
> margin around it. Background: plain flat solid bright green #00FF00, no
> ground plane, no shadow, no text. Avoid: bright saturated colors, neon, lens
> flare, bloom, 3D render, photo, blur, watermark, text.

Result note: on-brief; the root mud patch is part of the sprite (good — it
*is* the shadow pool monsters lurk in). Rust hook survives at 2 px.
