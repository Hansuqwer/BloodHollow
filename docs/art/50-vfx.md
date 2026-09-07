# VFX briefs (bible §12, Helbreath combat-storytelling lock) — B7 spec

Rules (from `helbreath/readability-rulebook.md` R-FX): ≤ 0.6 s hit FX;
persistent = ground decal; FX covers ≤ 40 % of caster body; reserved ramp on
magic only; colour language iron/red · ash-blue · bone-white · gold · violet.
Frames are 8×8 – 64×64, listed as `size × frames @ fps`. Everything is a plain
sprite strip (`anim.json` with `dirs:1` is **not** accepted by `loadAtlas` —
VFX ship as their own strip loader or as 8-row sheets with identical rows;
T-ART-04 covers this).

## Swings (iron/red)

| # | FX | Spec |
|---|---|---|
| 1 | Basic swing arc (all melee) | 32×32 × 3 @ 15 · iron-grey crescent, 1 px bone edge, f1 = contact, f2 fades. Rotates per dir (8 variants or engine rotate — nearest rotate shimmers; ship 8). |
| 2 | Power Swing arc | 48×32 × 4 @ 15 · wider crescent, f1 double-line, f2 dust puff at the floor; red-caps "Power-Swing!" callout (kind 5) fires with f1. |
| 3 | Hit flash | body swap, 2 frames: bone-white silhouette → palette (T-066). Not a sprite — a shader/tint on the target. |
| 4 | Crit flash | hit flash + 16×16 × 2 orange "!" spark burst at the wound point (kind 2 colour `255,120,40`). |
| 5 | Cleave proc | 32×32 × 3 · second crescent crossing the first (X), iron. |
| 6 | Sunder proc | 16×16 × 3 · cracked-plate shards falling from the target, iron. |
| 7 | Graft proc | 16×16 × 3 · red stitches jumping to the Ravager (leech), arterial-lite `#6a1a22`. |
| 8 | Gore Hook (later) | 48×16 × 4 · chain + hook, pulls; hook is iron, chain 1 px. |
| 9 | Execute (later) | 32×48 × 3 · vertical cleaver drop, arterial edge line f1. |

## Casts

| # | FX | Spec |
|---|---|---|
| 10 | Cast ring (all casters) | 48×24 ground ellipse × 3 @ 12 · HB's white-blue ring: bone-white core, ash-blue rim; f0 appears, f1 full, f2 fades; sits under the caster at feet y. |
| 11 | Firebolt (Gc ch5) | projectile 24×12 × 2 @ 12 (ember comet, tail 5 px alternating) + impact 24×24 × 3 · ember `#c8622a` core, iron edge, red-caps "Firebolt!" callout on cast. Proven in the style tile. |
| 12 | Blood Bolt (Gravemother) | projectile 32×16 × 2 + impact 32×32 × 3 · **arterial** `#8e101c→#d8302a` comet, matte spatter on impact (leaves decal #24); callout kind 9 `(235,40,160)` is the *engine's* current colour — flag: the bible says arterial; keep engine's magenta-red for the text, arterial for the sprite (text must differ from blood decals to read). |
| 13 | Corpse Explosion (later) | 48×48 × 4 · rib burst outward, 6 bone chunks + matte red disc decal; violet 1-frame flash at f0 (curse licence). |
| 14 | Blood Curse (later) | 16×16 × 4 glyph over target's head, **violet** `#6B4A8A` (highlight `#A884C4`) dithered to `#3a2a50`; persistent icon in the debuff strip. |
| 15 | Bone Wall (later) | 64×48 × 3 rise + static frame · five femurs punch up from the ground, bone-white, dust puff. |
| 16 | Wither (later) | ground decal 64×32 × 3 states · violet-black rot creeping across a tile; persistent. |
| 17 | Mend / Mass Mend (Cul ch2/ch7) | target: 16×24 × 4 rising bone-white motes (kind 8 green text `90,230,120` — flag: text is green, motes are bone-white; both allowed, text follows engine); Mass Mend adds cast ring #10 in bone-white at 64×32 under the Cultist. |
| 18 | Bless (ch3) | 24×8 × 3 gold ring over the head, then 12×12 gold glyph persists as buff icon; callout kind 10 gold `255,205,40`. |
| 19 | Ironskin (ch4) | 32×48 × 2 · grey plate outline flashes over the body silhouette (ash-blue), then 1 px iron rim while active (palette state, not glow). |
| 20 | Chorus (ch6) | 64×32 × 4 · three concentric gold rings on the ground expanding outward; callout kind 13. |
| 21 | Haste (ch8) | 16×16 × 2 · ash-blue speed lines behind the boots on each walk frame while active; callout kind 14 orange `255,140,30`. |
| 22 | Hymn of Teeth / Sanctuary (later) | Hymn: 32×32 × 3 gold jaw glyph + 3 note marks; Sanctuary: **ground decal** 96×48, gold ring + hymn glyph, 2-frame breathing pulse, persistent 10 s. |
| 23 | Raise Skeleton / Exorcise (later) | Raise: 32×48 × 4 skeleton climbs from a bone ring (uses ghoul-flesh bones); Exorcise: 32×48 × 3 gold flash + cowl blows away. |

## Gore & death

| # | FX | Spec |
|---|---|---|
| 24 | Blood decals | 6 shapes (spatter, pool, drag, spray-arc, handprint, boot-track) × 3 decay (fresh matte `#3a080c` → dried `#2a1014` → stain `#1e1418`) at 32×16/48×24/64×32; alpha-cut, no outline; 10 min life (engine T-ART-09). |
| 25 | Gib bursts | per mob family: 32×32 × 3 · 4–6 chunks arcing out + one decal #24 pool; chunks come from the family palette (vermin grey-pink, ghoul, hound, grave-goods bone, widow chitin, choir-wax). |
| 26 | Corpse stages | per-mob corpse frame (die last frame) → 3-stage decay decal (fresh, picked-clean ribs, stain) — designed; engine ships 0.6 s vestige. |
| 27 | Petrify-fade vestige | 32×48 × 2 ash overlay + engine grey; ash flakes fall 3 px. |

## Boss telegraphs & world

| # | FX | Spec |
|---|---|---|
| 28 | Gravemother rot rings | ground decal 192×96 × 3 stages (stain → violet veins → burst), 3-tile diameter; violet-black. |
| 29 | Bell shockwave ("rings herself") | 128×64 × 4 · expanding ash-blue ring + 3 bell-note glyphs, dust; add-spawn flash 24×24 × 2 bone-white at each spawn point. |
| 30 | Torch / candle / anvil light pools | painted decals 96×48 (torch) · 64×32 (candle cluster) · 128×64 (anvil) · warm `#c8924a` → `#5a3a20` dither, static; plus a 2-frame flicker sprite 8×16 on the source. |
| 31 | Night ambience | wisp glint 4×4 × 2 (bone-white, fields only), moth 4×4 × 2 around lights; no additive. |

Count: **31** (bible asked ~25; the 6 extra are the later-kit casts, ordered
last in production).

Prompt skeleton (bible §13 VFX): `"1999 MMORPG spell effect sprite frame,
hand-drawn pixel art, [colour family] palette only, [shape], hard edges,
dithered falloff, no glow, no bloom, solid #00FF00 background, single frame,
frame [n] of [N]"` — generate the key frame only (contact/peak), draw the
in/out frames by hand from it; AI is bad at coherent 3-frame strips and the
frames are 8–48 px anyway.

Callout font (§12, R-TEXT): red-caps bitmap, 1 px black outline, glyph set
A–Z 0–9 ! - ' . : + / (cell 7×11 per rulebook R-TEXT; a 5×7 parity draft
also exists — decision D4, `docs/art/PARALLEL-ROADMAP.md §5`; drafts in
`docs/research-notes/style-tile/export/font/`, `tools/atlaspack/bhfont.py`),
exported as `icons/ui/callout_font.png` + `.fnt`; night plate 2 px `#0c0a0a`
α160. Replaces raylib `DrawText` in `drawFloaters` (proposed T-ART-13; no
client bitmap-font path exists at `game.cpp:904`).
