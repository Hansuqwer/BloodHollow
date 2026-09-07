# Monster briefs (bible §7, Helbreath sprite lock) — B3/B4 production spec

Template per mob (all fields filled): **silhouette · palette (5–8 named) ·
idle/walk motion · attack tell (contact on frame 2 of 3) · death/gib ·
night cue · era ancestor · silhouette-test note · cell/sheet · prompt line.**

Common-mob budget (GDD §11): 8 dirs × (walk 4 + attack 3 + die 3) = **80
frames** → sheet 10 cols × 8 rows = **320×384 px** at 32×48 (≤ 256×384 in the
bible is arithmetically impossible at 10 columns; amended to **≤ 320×384**,
still ~20 KB PNG). Elites add hurt 2f (12 cols). Boss = 64×64 × (4+3+4+2+4+4)
= 21 cols → **1344×512**; bible's ≤ 512×512 is amended to **≤ 1344×512** or
split into two PNGs (loader is one-PNG-per-atlas, so one file).

Shared family palettes (`bhpix.build_palette`, ≤ 32 each): **vermin**
(1001, 1004, Old Maw) · **ghoul-flesh** (1002, Crypt Revenant, skeleton pet,
Mine Wretch) · **hound** (1003, Pale Sow) · **grave-goods** (1005, 1008, 1010) ·
**widow** (1006, Lantern Spider, Red Widow) · **choir-wax** (1007, Pale
Cultist, Cantor Vex, Bell Ringer, Gravemother) · **night** (Wraith, Bloodfiend,
Banshee — the only family with accent-ramp entries).

Every prompt = `PREFIX` (bible §13) + the line below + `NEGATIVE`; generate
S, SE, E natively, derive SW/W/NW/N/NE by img2img (denoise ≤ 0.35) and
hand-fix gear sides. Contact frame = frame index 1 (0-based) of the 3.

## Shipped roster (`mobs.h`, MUST exist)

### 1001 Marsh Rat — L1, 30 hp, tutorial mob
- **Silhouette:** low teardrop, tail = 40 % of length, head down.
- **Palette (vermin):** wet slate `#4a4a50`, mud `#5a4a3a`, grey-pink belly
  `#9a7f7e`, bone teeth `#cfc6b4`, black ichor `#141014`, rust eye `#7a2a1e`.
- **Walk 4f:** skitter — body bobs 1 px, legs alternate, tail whips 2 px.
- **Attack 3f:** rear 1 px → **bite lunge (contact)** → recoil.
- **Die 3f:** flop on side → bloat (belly +2 px) → burst decal (1 px ichor
  spatter, gib = 2 meat chunks + tail).
- **Night cue:** none (pathetic by design); eyes do *not* glint.
- **Ancestor:** HB Giant Rat / L1 Rat — stayed pathetic in both.
- **Silhouette test:** vs Plague Bat (W) / Ghoul (tall S) / Hound (wedge): the
  only *low horizontal* shape at L1–5. Proven in style tile.
- **Cell:** 32×48, body 20 px. Sheet 320×384.
- **Prompt:** `PREFIX + "Marsh Rat, swollen drowned rat, matted spiked wet
  fur, bloated grey-pink belly, black ichor drool, naked tail, low skitter, 3/4
  south-facing, sickly pathetic, single creature, feet on ground" + NEGATIVE`
  (B0 plate exists: `docs/research-notes/style-tile/plates/marsh_rat_4x_raw.png`).

### 1002 Feral Ghoul — L3, 64 hp, drops Rusty Shank 4 %
- **Silhouette:** tall S-curve, arms past knees, nail-hands 3 px wide.
- **Palette (ghoul-flesh):** grey-pink `#8e7776`, bruise `#5a4a5e`, black
  nails `#141014`, exposed rib bone `#cfc6b4`, dried blood `#3a080c`, rag
  `#4a4238`.
- **Walk 4f:** hunched lope — head leads, arms swing opposite, 2 px bob.
- **Attack 3f:** wind-up (both arms back) → **double rake (contact, both
  hands forward, 1-frame arterial streak 6 px)** → hang.
- **Die 3f:** knees buckle → face-plant → still; **gib = torso burst** (ribs
  + 3 chunks + head).
- **Night cue:** none; ghouls are the *day* fear of the fields.
- **Ancestor:** HB Zombie / DE slayer-turned; Mir skeleton lope.
- **Silhouette test:** only tall thin S among L1–5; arms-past-knees is the key.
- **Cell:** 32×48, body 40 px. Sheet 320×384.
- **Prompt:** `PREFIX + "Feral Ghoul, starved hollowed human, grey-pink
  flesh, exposed ribs, long black nail-hands hanging past knees, hunched lope,
  rag loincloth, mouth agape, 3/4 south-facing, single creature" + NEGATIVE`.

### 1003 Hollow Hound — L5, 100 hp, drops Hound Fang 60 %
- **Silhouette:** forward wedge — head low and long, spine ridge, ribs.
- **Palette (hound):** matted black-brown `#2e2622`, grey-pink hide `#7e6a68`,
  skull half-mask bone `#d2c8b6`, rib bone, tongue `#6a2a30`, **eye glint
  `#c9bfae`** (painted, 1 px, S/SE/SW only).
- **Walk 4f:** pack trot — legs in diagonal pairs, head fixed (predator
  read), tail low.
- **Attack 3f:** crouch → **lunge bite (contact, body +4 px forward)** →
  land.
- **Die 3f:** legs fold → roll → still (skull mask slides off = corpse frame).
- **Night cue:** the painted eye glint is the *only* pixel above luma 180 on
  the sprite — under the night overlay it's the first thing seen. Night
  behaviour (+1 aggro, ×1.15 dmg, T-061) needs no art change.
- **Ancestor:** HB Hellhound / L1 Werewolf-hound; DE wolf-form.
- **Silhouette test:** the only *forward-leaning quadruped*; rat is low-round,
  Pale Sow (later) is barrel-round.
- **Cell:** 32×48, body 26 px tall × 30 wide (uses full cell width). Sheet 320×384.
- **Prompt:** `PREFIX + "Hollow Hound, gaunt undead mastiff, ribs visible
  through grey-pink hide, half of the skull exposed as a bone mask, lean
  forward-leaning pack posture, low head, one pale glinting eye, 3/4
  south-facing, single creature" + NEGATIVE`.

### 1004 Plague Bat — L2, 22 hp, swarm (maxAlive 9–10)
- **Silhouette:** W with a drop — wings up, bloated belly hanging.
- **Palette (vermin):** membrane `#3a3236`, fur `#4a4a50`, **greenish bloat
  belly `#5a6a48`**, bone claws, black eyes, ichor.
- **Walk 4f (flight):** 2-frame wing blur is acceptable (§7): up-blur / down-blur
  alternating with 1 px vertical bob; feet anchor = belly bottom at y=30 (it
  *hovers* — the contact shadow stays at y=45 so it reads airborne).
- **Attack 3f:** dip → **belly-slam bite (contact)** → flap up.
- **Die 3f:** wings fold → drop → **wet smack decal** (green-black splat 12×6;
  no gib — too small).
- **Night cue:** none; the "die red-faced" exam mob must be *boring* to look at.
- **Ancestor:** HB Giant Bat / DE bat-form.
- **Silhouette test:** the only airborne W. Shadow separation from body is the
  read.
- **Cell:** 32×48, body 18 px, hover. Sheet 320×384.
- **Prompt:** `PREFIX + "Plague Bat, fat diseased bat in flight, ragged dark
  membrane wings spread in a W, swollen greenish plague belly, tiny bone
  claws, 3/4 view from slightly above, single creature, hovering" + NEGATIVE`.

### 1005 Bonepicker Gnoll — L7, 160 hp, drops Pit Blade 3 %, bounty 250
- **Silhouette:** hunch + club — knuckle-walk mass with a femur club over the
  shoulder (the 3 permitted protrusions: club, snout, hunch).
- **Palette (grave-goods):** hyena tan-grey `#6a5e4e`, mane soot `#2b2628`,
  **rib-cage armour bone `#cfc6b4`**, femur club, grave-wire rust `#6a3a26`,
  gum pink `#8e5a5e`, one gold tooth-ring `#a88a4a` (≤ 4 px, grave-goods not
  choir-gold — it's *loot*).
- **Walk 4f:** knuckle-walk — front knuckles touch ground on frames 0/2.
- **Attack 3f:** **laugh tell** (head back, jaw open, frame 0) → **club
  overhead smash (contact)** → drag back. The laugh is a 1-frame tell — era
  telegraphs are short.
- **Die 3f:** club drops first → sits → topples backward (corpse frame shows
  the rib-armour open). Gib = rib cage + club + 2 chunks.
- **Night cue:** none.
- **Ancestor:** HB Orc / Mir Oma warrior — the mid-field anchor in both.
- **Silhouette test:** the only *asymmetric* silhouette (club right side); vs
  Sexton (T) the gnoll is a hunch, Sexton is upright.
- **Cell:** 32×48, body 42 px. Sheet 320×384.
- **Prompt:** `PREFIX + "Bonepicker Gnoll, hyena-headed man wearing grave
  goods, a human rib cage strapped on as armour with rusted wire, a femur
  club over the right shoulder, hunched knuckle-walking stance, matted soot
  mane, laughing snout, 3/4 south-facing, single creature" + NEGATIVE`.

### 1006 Charnel Widow — L9, 220 hp, drops Widow Silk 35 %, bounty 300
- **Silhouette:** disc + 8 sticks — coffin-lid abdomen is a flat oval, legs
  are 1–2 px lines with a knee kink.
- **Palette (widow):** abdomen wood `#3a2e26` with brass coffin nails
  `#8a6a3a`, chitin black `#141014`, joint grey-pink, **shroud-thread silk
  `#a9a29a`**, 8 eyes bone-white, venom `#4a5a3a`.
- **Walk 4f:** slow deliberate — legs move in two tetrapod sets, body glides
  0 px vertical (spiders don't bob).
- **Attack 3f:** raise front pair → **fast strike (contact, fangs + 1-frame
  silk line 8 px)** → settle. Fast strike after slow walk = the Widow's
  personality (§7).
- **Die 3f:** legs curl in → flip → coffin-lid abdomen splits (corpse frame
  = open lid). Gib = lid halves + legs.
- **Night cue:** eyes glint (8 × 1 px) like the Hound.
- **Ancestor:** HB Giant Spider / Mir cave spider; Lantern Spider is her
  night cousin.
- **Silhouette test:** the only radial shape at L5–12.
- **Cell:** 32×48 with **legs spanning the full 32 px**; body 22 px. Sheet 320×384.
- **Prompt:** `PREFIX + "Charnel Widow, bloated grave spider, its abdomen is a
  fused wooden coffin lid with brass nails, black chitin legs with grey-pink
  joints, trailing grey shroud-thread silk, eight bone-white eyes, low
  deliberate stance, 3/4 view from above, single creature" + NEGATIVE`.

### 1007 Gravecaller (mob) — L11, 260 hp, drops Bone Plate 2 %, bounty 400
- **Silhouette:** triangle robe + censer chain — narrow shoulders, hem trails
  mud, a bell-censer swinging on a 6 px chain.
- **Palette (choir-wax):** robe ichor-black `#1e1a20`, **wax mask `#d9cdb4`**,
  chain iron `#4a4e58`, bell bronze `#6a5a3a`, censer ember `#c8622a` (warm
  light, permitted), grave-mud hem `#3a2f26`, ichor veins `#141014`.
- **Walk 4f:** glide — robe hides legs, 1 px bob, hem drags a mud smear
  (1-frame decal every 4th step optional).
- **Attack/cast 3f:** censer back → **censer arc forward (contact; arc of 5
  ember dots)** → recover. Mob "attack" is the cast (melee stat in `kMobs`
  resolves it as a swing).
- **Die 3f:** mask falls (1 frame, mask lands as a decal) → robe collapses
  → empty robe heap (corpse frame). Gib = mask + bell + robe scraps.
- **Night cue:** censer ember is a permanent 2 px warm point.
- **Ancestor:** HB Dark Elf caster / DE priest; the player Gravecaller shares
  the mask motif (§9) — mob version has *no face* under the mask.
- **Silhouette test:** the only *robed* enemy in fields/crypt with a swinging
  attachment.
- **Cell:** 32×48, body 44 px. Sheet 320×384.
- **Prompt:** `PREFIX + "Gravecaller plague priest, hollowed corpse in black
  ichor-stained scholar robes, featureless pale wax mask, swinging a bronze
  bell censer on an iron chain, ember glow in the censer, robe hem trailing
  grave mud, 3/4 south-facing, single figure" + NEGATIVE`.

### 1008 Revenant Sexton — L12, 420 hp, drops Revenant Ash 100 %
- **Silhouette:** T — upright armoured torso, rusted spade-halberd held
  vertical, key-ring bulge on the belt.
- **Palette (grave-goods):** rust plate `#5a3a2a`, iron `#4a4e58`, grave
  cloth `#3a3632`, bone, keys brass `#8a6a3a`, cold flesh grey-pink, one
  arterial line where the spade is stained.
- **Walk 4f:** heavy **drag step** — frames 0/1 lift, 2/3 drag the left foot
  (asymmetric; do not mirror).
- **Attack 3f:** halberd overhead → **spade split (contact; iron-grey arc + 1
  white flash)** → wrench free.
- **Die 3f:** kneel on halberd → armour slumps → falls sideways. Gib = plates +
  keys + spade head.
- **Night cue:** none.
- **Ancestor:** HB Skeleton Knight / L1 Death Knight.
- **Silhouette test:** vs Gnoll (hunch) the Sexton is *upright*; vs Elite the
  Sexton has *no* white bindings and is 1.0×.
- **Cell:** 32×48, body 46 px (halberd may exit the top by 2 px — allowed for
  the 15 % rule). Sheet 320×384.
- **Prompt:** `PREFIX + "Revenant Sexton, armoured undead gravekeeper,
  rusted plate over grave cloth, holding a rusted spade-bladed halberd
  upright, iron key ring on the belt, cold grey-pink face under a hood,
  heavy dragging stance, 3/4 south-facing, single figure" + NEGATIVE`.

### 1010 Sepulcher Elite — L12 ×8, 380 hp, 8 camps ring the boss
- **Silhouette:** bigger T — Sexton family at **1.25×** with **chalk-white
  grave-binding strips** crossing the chest (≤ 8 % of pixels; the elite
  readability trim, §4.5).
- **Palette:** grave-goods family + binding white `#e6e0d4`.
- **Anims:** walk 4 / attack 3 / hurt 2 / die 3 (elite adds hurt). Hurt =
  flinch back 2 px + 1 white flash frame.
- **Night cue:** bindings are the brightest non-light pixels in the crypt.
- **Ancestor:** L1 named-elite variants (same sprite, bigger, tagged).
- **Silhouette test:** must read as "a Sexton, but wrong" — the bindings are
  the tell, size is confirmation.
- **Cell:** **40×60** (1.25× of 32×48), feet y=52 (`anchorY` in JSON, see
  VERIFY #22). Sheet 12 cols → 480×480.
- **Prompt:** Sexton prompt + "larger tomb-warden variant, chalk-white
  linen grave bindings wrapped across the chest and arms".

### 1009 The Gravemother — BOSS L14, 700 hp, Blood Bolt 6 tiles / 26 t
- **Silhouette:** **bell dome with a torso** — a great cracked bronze bell
  (≥ 48 px wide) as the lower body; a drowned matriarch's torso rises from the
  bell mouth, arms long, hair like weed. 5 protrusions: two arms, head,
  clapper-chain, bell lip crack.
- **Palette (choir-wax + arterial licence):** bell verdigris bronze
  `#4a5a4e`/`#6a5a3a`, drowned flesh grey-blue-pink `#8a8290`, weed hair
  `#2e3a2e`, grave-lace `#d9cdb4`, black ichor veins, **arterial `#8e101c`
  only in the Blood Bolt and the phase-3 veins**.
- **Anims (64×64, feet/anchor y=58):** walk 4 (the bell *drags*, leaving a
  2-tile rot decal every 2 tiles — engine decal layer, VERIFY #21) · attack 3
  (arm sweep, contact f1) · **cast 4** (Blood Bolt: both hands to chest →
  arterial bolt leaves f2) · hurt 2 · die 4 (bell cracks open, torso slides
  in, bell settles — the corpse frame is a split bell) · **summon 4 ("she rings
  herself"):** torso grips the bell lip → heaves → bell tilts + clapper strikes
  (shockwave ring FX) → adds spawn flash on the ring.
- **Phases (T-064 ships one kit; art supports 3 by *palette state*):** P1 as
  above · P2 ichor veins light arterial on the torso (palette swap of 3
  colours, permitted because it's *curse light*) · P3 bell crack glows ember.
- **Ground rot telegraph decals:** 3-tile ring, 3 stages (stain → veins →
  burst), matte violet-black `#2a1a30` → this is *curse*, so violet is licensed.
- **Night cue:** the bell crack ember (P3) is the warm light of the nave.
- **Ancestor:** HB Abaddon / Mir Zuma Taurus king — big, slow, telegraphed;
  DE for the arterial cast.
- **Silhouette test:** dominates a crowd at 64 px wide while players (14 px
  shoulders) stay visible beside — never *over* — her; her attacks are
  arm-sweeps that stay within her own cell.
- **Sheet:** 21 cols × 8 rows × 64×64 = 1344×512.
- **Prompt:** `PREFIX + "The Gravemother, drowned undead matriarch fused
  into a great cracked verdigris bronze bell, her torso rising from the bell
  mouth, long arms, weed-like black hair, grave lace, black ichor veins,
  clapper chain hanging, massive and slow, 3/4 south-facing, single
  creature, boss" + NEGATIVE`.

## GDD-target roster not yet shipped (design now, implement when `kMobs` rows land)

| Mob | Family | Key silhouette | One-line brief | Night/accent |
|---|---|---|---|---|
| Pale Cultist (field caster) | choir-wax | robe + **choir mask** (smooth oval with a tooth-hymnal sigil) + staff-crook | living zealot, bone-white robe with incense-gold hem trim (the Synod's colours *on an enemy* — tells players the Synod is not clean); cast = open palm up | choir-gold glyph on cast |
| Mine Wretch | ghoul-flesh | ghoul with a **lantern for a jaw** (iron cage + candle) | undead miner, pick over shoulder, one boot | permanent warm 3 px light + pool decal |
| Lantern Spider | widow | widow with a **glowing abdomen** (§4.3 self-illumination) | the mine's night hunter; abdomen = paper-lantern amber `#c8924a` dithered, 6 px | the abdomen is the light; body reads by its silhouette against its own glow |
| Mud Golem | grave-goods | **boulder pile with fence-post arms** | fields heavy, 40×56 cell, slow 4f walk with mud drip decals | none |
| Crypt Revenant | ghoul-flesh | ghoul in **rotted burial finery** | crypt upgrade of Feral Ghoul: same bones, tailcoat rags + a coin on one eye | none |
| Grave Banshee (elite) | night | tall, **no legs — hem dissolves**, mouth = the widest thing on her | wail = visible expanding ring FX (3f, ash-blue) + hurt 2f | translucent (alpha 200 body, hard outline kept) |
| Bell Ringer (summoner elite) | choir-wax | hunched with a **handbell bigger than its head** | rings → 2 bell-adds (mini 1001-family "bell rats"); summon 4f | ember in the bell |
| Wraith (night-only) | night | **hooded nothing** — cowl + two claw hands, hem dissolves | additive-friendly: palette holds 3 bone-blue tones + outline; 4f walk is a drift | +50 % XP mob; must read *because* of the night, not despite it |
| Bloodfiend (night-only) | night | ghoul-flesh S-curve with **arterial red licence** | veins `#8e101c`, bite = drain beam (DE), applies Blood Curse | the only common mob with red on the body |
| Old Maw (named, fields) | vermin | Marsh Rat family at 1.5×, **one eye, jaw hangs open** | den decal in fields; unique silhouette + overhead name | none |
| The Red Widow (named, mine) | widow | Charnel Widow 1.5× with **red hourglass** on the coffin lid | webs a chamber (decal set) | glinting eyes |
| Cantor Vex (named, crypt) | choir-wax | Gravecaller family with a **bell rack on his back** (4 bells) | rings a side chapel; cast = sequential bell strikes | ember censer |
| The Pale Sow (v0.2 world boss) | hound | **barrel-round plague beast**, teat-row of bells, 64×64 | Blood Moon roamer | arterial + choir-gold (it's *holy* to the Synod) |

Elite/named rule (bible §7): elites = same family + white/gold trim + 1.25×;
nameds = unique silhouette + name; never palette-swap alone.
