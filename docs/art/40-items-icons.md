# Weapons, items & icons (bible §10–§11) — B7/B8 production spec

## §10 Weapon silhouettes (in-hand attachments, 8 dirs, +15 % scale)

Drawn as paper-doll attachments (separate 32×48 layer per dir, pivot = right
hand). Refine tiers are *paint*, not glow: +0 base · +5 tier-2 paint (edge
highlight ring) · +9 tier-3 paint (arterial edge on weapons, ember seams on
armour) · **+10 (cap, T-060) = unique silhouette add** (a hook, a bell, a
skull). Glow overlay from +5 is an additive 32×48 sprite (T-ART-11), 2 frames,
alpha ≤ 90.

| Item (id) | Base silhouette | +5 paint | +9 paint | +10 add |
|---|---|---|---|---|
| Rusty Shank (2001) | 8 px pitted blade, rag grip | pale edge highlight | arterial edge | a second nail through the grip |
| Pit Blade (2002) | 12 px cleaver, notched, gnoll teeth on the spine | teeth whitened | edge arterial, spine ember dots | a jaw-bone guard |
| Hide Armor (2101) | widow-silk-stitched hide vest, grey-pink | silk stitches white | seams ember | a coffin-nail collar |
| Bone Plate (2102) | rib-and-scapula plates over cloth, wax-sealed joints | wax gold-ish (grave-gold `#a88a4a`, not choir) | ember seams | a bell at the sternum |

Later kit (design only): Butcher Cleaver (Ravager iconic, 14 px), Bell
Censer (Gravecaller off-hand), Choir Crook (Cultist staff, crook holds one
candle), Ashen kite shield, Synod visor helm.

## §11 Icons — 32×32 UI, 24×24 hotbar, 1× and 2× (nearest; 2× is a **redraw
pass**, not a resize, for the 8 shipped skills; the rest may resize)

Backing colour language (T-066 lock, verified in floaters): **iron/red** =
damage · **ash-blue** = utility · **bone-white** = heal · **gold** = holy ·
**violet** = curse. Backing = 2 px inner bevel in the family colour on a
`#151013` plate; the glyph sits on top. Cooldown sweep = engine (radial
dark wedge); charges = 6×6 numeral corner.

### Skill icons (8 shipped channels first, then GDD set)

| ch | Skill (class) | Family | Glyph | Prompt subject |
|---|---|---|---|---|
| 1 | Power Swing (Rav/Cul) | iron/red | cleaver mid-arc, 3 motion lines | "oversized notched cleaver mid-swing, three iron motion lines" |
| 2 | Mend (Cul) | bone-white | open palm, two rising motes | "open palm with two rising bone-white motes" |
| 3 | Bless (Cul) | gold | choir-gold ring over a small hymnal | "gold halo ring above an open hymnal with a tooth" |
| 4 | Ironskin (Cul) | ash-blue | grey plate with three rivets | "iron breastplate with three rivets, blue-grey" |
| 5 | Firebolt (Gc) | iron/red | comet with ember tail | "comet-shaped firebolt with a tapered ember tail" |
| 6 | Chorus (Cul) | gold | three open mouths in a triangle | "three singing mouths arranged in a triangle, gold" |
| 7 | Mass Mend (Cul) | bone-white | palm + ring underneath | "open palm above a pale ground ring" |
| 8 | Haste (Cul) | ash-blue | boot with two speed lines | "worn boot with two speed lines, ash-blue" |
| — | Cleave / Sunder / Graft (aura procs, kind 7 callout) | iron/red | X of blades / cracked plate / stitched flesh | proc icons for the log only |

GDD set, designed now, shipped when channels exist (§11 P4/P5/P6):
Ravager — Second Wind (fist to chest), Gore Hook (hook on chain, iron), Bloodrage
(veins, **arterial**), Execute (down-arrow cleaver); Gravecaller — Corpse
Explosion (burst rib cage), Blood Curse (violet eye), Bone Wall (five bones
upright), Shroud (violet veil), Wither (violet hand over a tile); Cultist —
Hymn of Teeth (jaw + notes, gold), Sanctuary (gold ring decal), Raise Skeleton
(bone rising from a ring), Ash Ward (grey ward glyph), Exorcise (gold flash on
a cowl). Aura tiers I–V (Edge Rite → Crimson Pact): one **rite-mark** glyph
in 5 progressive states (single cut → five cuts in a circle), iron ramping to
arterial at V.

### Item icons

| Item | Icon read |
|---|---|
| Blood Vial (3001) | glass vial, red-black contents, wax stopper; 3 fill states by stack tens |
| Blackiron Ore (5001, reserved) | dull iron-blue nugget, one glint, on rag |
| Fodder tiers (5101–5103, reserved) | broken shank / broken plate / broken bell — "food for the anvil" |
| Rat Pelt (4001) | flat grey-brown pelt with tail |
| Ghoul Finger (4002) | black-nailed finger, grey-pink |
| Hound Fang (4003) | yellowed fang, root end dark |
| Widow Silk (4004) | grey shroud-thread skein |
| Revenant Ash (4005) | grey ash cone on a rag, one ember |
| Gold | stacked black-iron coins with a nail stamped in (Vessalia's coin is *iron*, not gold — matches the ash grade; flag for lore) |
| Weapons/armour | drawn at 32×32 as the same objects as the in-hand attachment, painterly-real like the HB bag |

### UI chrome

- **Karma band glyphs** (6×6, left of name): lawful = closed lantern · neutral
  = plain dot · chaotic = split drop. Tint follows code colours.
- **Buff/debuff strip** (16×16): Bless (gold ring), Ironskin (plate), Haste
  (boot), Chorus (mouths), Blood Curse (violet eye), Wither (violet hand),
  Sanctuary (gold ring), Bloodrage (arterial veins), Night (crescent, ash-blue
  — shown 21→05 to explain the +15 %).
- **Anvil modal** (Mir theatre): 3 chrome states — pending (dim, ember pulse
  2f) · success (choir-gold edge flash 2f, +N badge) · destroy (arterial edge
  flash 2f, icon cracks 2f, slot empties). Panel plate `#0c0a0a` α 200, red
  trim `(150,30,30)` from the shipped HUD.
- **Bounty board panel**: parchment-on-plank, quarry stencil, DE-style kill
  counter for the claimable count.
- **NPC dialog frame**: Soma silver filigree, 96×96 portrait slot, HB
  wood/brass for shop lists.
- **Death vestige**: engine draws 0.6 s grey; art adds a 32×48 **petrified
  ash** overlay (2 frames) so "petrify-fade" reads as ash, not tint.

Prompt skeleton for icons: `"1999 MMORPG inventory/skill icon, 32x32 pixel
art, hand-drawn, 1px dark outline, [family] colour backing plate, [glyph],
muted dark palette, no text" + NEGATIVE`. Generate at 8× (256×256) and
`fit_to_cell` down with NEAREST; redraw the 8 shipped at 2× by hand.
