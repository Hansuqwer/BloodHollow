# NPC & player briefs (bible §8–§9) — B5/B6 production spec

## NPCs (8; static furniture-class, wire kinds 64+, 8-dir idle 4f + 96×96 portrait)

Portrait spec: 96×96, painterly (Soma modal trim owns the frame, so the
portrait itself is *paint*, not pixel art — it sits inside the ornate-silver
frame at 1×), ≤ 32 colours, bust to mid-chest, 3/4 view, lit from one candle
(warm key, cold fill). Bark = one line the dialog opens with.

| # | NPC | Kind | Sprite brief (32×48 unless noted) | Portrait | Bark |
|---|---|---|---|---|---|
| 1 | **Marta** (vendor, Thornwall square) | 64 (ships) | Broad, flour-dusted apron over ash-grey wool, key ring + tally-sticks on belt, sleeves rolled, forearms like a smith's; idle 4f = wipes hands. Stall is separate furniture (see terrain). | Two portraits: *neutral* (tired half-smile) and **sneer** (for `karmaBand==2`: lip curled, eyes down at your hands) — portrait swap, not sprite swap. | "Coin first. Then you can bleed on my counter." |
| 2 | **The Bonesmith / Anvil priestesses** (twins) | 67 | Two soot-veiled sisters flanking the anvil (one 64×48 cell holding both, or two 32×48 with a shared anchor — choose the 64×48 so they never desync): left holds a hammer, right holds tongs; ember rim light on the veil edges (warm, permitted). Idle 4f: hammer sister taps, tongs sister turns a bar. | One portrait, both faces half-lit by the coals, veils identical, eyes different (one hollow, one bright). | "Steel remembers who fed it. What will you feed it?" |
| 3 | **Chapel Confessor** | 68 | Ashen Compact ash-robe, nail-through-leaf sigil on the stole, a nailed-plank prayer board under one arm; idle 4f = turns a page. Lawful respawn anchor, so he stands on the chapel wood floor tile. | Gaunt, kind, soot on the fingers; a bell rope visible behind. | "The Curse leaves with the blood. Kneel; it hurts less." |
| 4 | **Bounty Board** (furniture) | 66 (ships) | 32×48 plank board on two posts; **2 states**: *has-bounty* (3 notices, one blood-stained, the top notice carries the quarry silhouette stencil — rat/gnoll/widow/gravecaller/bell) and *empty* (nail holes, one torn corner). No idle. | — | (panel text) "THE BOARD WANTS: …" |
| 5 | **Smugglers' Cove fence** | 69 | Hooded, wax-seal ledger, dockside grime on the hem, Marrowgate rope belt; idle 4f = seals a page (tiny warm wax drip). Buys from chaotics at 60 %. | Hood shadow, only the mouth and a gold tooth lit; ledger seal in incense-gold. | "I don't ask where. I ask how much." |
| 6 | **Town Guard ×2 factions** | 70 Ashen · 71 Synod | Same base body (the player m-base with helm): **Ashen Compact** — ash-grey tabard, iron kettle helm, nail-through-leaf sigil, spear + kite shield; **Pale Synod** — bone-white tabard, wax-sealed visor helm, open-hymnal-with-tooth sigil, halberd. Idle 4f = shifts weight; they are the chaotic-hunting threat, so shoulders 14 px and weapons at +15 %. | Two portraits: helmets on, faces unreadable — the *sigils* are the faces. | Ashen: "Move along, red." · Synod: "The Choir sees your name." |
| 7 | **Pledge Registrar** (Marrowgate) | 72 | Zealot clerk, bone-white robe, **incense-gold ink stains** on fingers and cuff (choir-gold licence: it's blessing-ink), quill + a chained ledger; idle 4f = dips quill. Handles bloodpledge creation (CHA ≥ 20 + 100k). | Thin, ink under the eyes like kohl, hymnal open. | "A hundred thousand and a name. The Synod keeps both." |
| 8 | **Castle Steward** (Weeping Castle) | 73 | Neutral grey robes with a **weep-stain motif** (vertical dark streaks from the shoulders), key on a chain, no sigil — he serves whoever holds the keep; idle 4f = polishes the key. | Old, wet-eyed, stone wall behind him streaked the same way. | "The castle does not care who you are. Only that you pay." |

Sheets: idle 4f × 8 dirs = 32 frames, 128×384 (or 256×384 for the twins).
Palette family **npc-ash** (1, 3, 6a, 8), **npc-synod** (5, 6b, 7), **npc-ember**
(2) — three strips.

Prompt skeleton: `PREFIX + "[name], [one-line above], 3/4 south-facing,
standing idle, full body, feet on ground plane, single figure, era MMORPG
NPC" + NEGATIVE`. Portrait: `"painterly bust portrait, 1999 Korean MMORPG
NPC dialog portrait, dark horror, single candle key light, muted palette,
[face brief], ornate silver frame NOT included, plain dark background" +
NEGATIVE`.

## Players (3 classes × m/f × 3 skin tones, one shared base per sex)

**Paper-doll layers (MVP = tint/attach):** base body (m/f) → skin tone (3
palette swaps: pale `#d6c6be`, sallow `#b8a48a`, weathered-tan `#8e6e52`) →
class overlay (robe/armour) → weapon attach → trim colour (faction/party
≤ 8 %). All 8 dirs, full set idle 1 / walk 6 / attack 3 / cast 4 / hurt 2 /
die 4 / gib 3 = **23 cols × 8 rows = 736×384** per (class, sex) at 32×48
(D2 ruled 32×48 on 2026-09-08 — 736×384 fixed).

Base body rules (from B0): body 46 px; head 8 px; shoulder widths by class;
**the right hand is the weapon hand in every direction** (no mirroring);
feet y=42; contact shadow painted in cell.

### Ravager — melee DPS / off-tank (STR look)
- Heavy shoulders (14 px), butcher-chain belt with hooks, soot-grey gambeson
  with nailed rust plates, oversized cleaver-axe (blade ≥ 10 px). Weight
  forward in walk (torso leads by 1 px). B0 plate exists.
- Attack 3f: wind-up (cleaver behind) → **contact (cleaver forward, iron-grey
  arc + 1 white flash)** → follow-through. Cast 4f (Second Wind): fist to
  chest, hold, exhale, release — no light.
- Hurt 2f: stagger back 2 px + white flash. Die 4f: knee → fall forward →
  cleaver clatters (decal) → still. Gib 3f: torso burst, cleaver survives as a
  decal (players' weapons drop as ground items — the cleaver decal doubles).
- Female variant: same shoulders (this is armour, not anatomy), longer hair
  bound in a soot rag, same cleaver.

### Gravecaller — ranged burst / control (squishy read)
- Narrow shoulders (10 px), long ichor-stained scholar robe (hem to feet),
  **wax half-mask** on the upper face (the mob's full mask, halved — players
  are still people), bell-censer in the off-hand on a 5 px chain, a short
  bone knife in the weapon hand (Gravecaller cannot Power-Swing, `kits.h`:
  ch1 unlock 0 — the knife is for the silhouette, not a skill).
- Cast 4f: censer swing back → **arc forward (bolt leaves f2)** → hand jab →
  recover; Corpse Explosion variant = both hands down then up.
- Attack 3f (basic): knife jab, contact f1 (rare; keep cheap).
- Die 4f: mask falls (decal) → robe folds → face-down → still.

### Cultist of the Pale Choir — support
- Medium shoulders (11 px), choir robe (bone-white going grey at the hem)
  with the **open-hymnal-with-tooth** sigil on the chest, **two votive candles
  at the shoulder-clasps** (2 px warm points each — the §4.3 self-illumination;
  they are the party's night marker), staff-crook in the weapon hand.
- Cast 4f — two gestures: **Mend/Bless/Ironskin/Haste** = open palm up (staff
  planted); **Chorus/Mass Mend/Sanctuary** = both arms raised (staff across).
  Choir-gold glyph over the head on f2.
- Attack 3f: staff bash (Cultist *can* Power-Swing: ch1 unlock 1), contact f1.
- Die 4f: candles gutter first (f0: the two points go out) → kneel → fall.
  That 1-frame candle-out is the Cultist's death tell for the whole party.

### Karma / faction on players
- Sprite unchanged by karma (GDD §5, code: `karmaBand` tints the name only).
- One trim colour ≤ 8 % of pixels: Ashen ash-grey `#8a8682` / Synod
  bone-white `#e6e0d4` / unsworn none. Party green never on the sprite.

### v0.2 Vampire race — silhouette direction (design only)
- Second spine: hunched idle, arms forward-hanging, no weapon slots; claws
  are hands (3 px), fangs read only in the portrait. Jewelry ×6 = 1 px glints
  on wrists/neck, night-visible. Bat-dash = 2f blur. The m/f base bodies must
  keep the shoulder joint at the same pixel so the hunched overlay can reuse
  the walk cycle's leg frames — **do not paint arms into the torso layer.**

## Sheet layout (all humanoids, one PNG, per-anim offset blocks)

```
cols:  idle[1] walk[6] attack[3] cast[4] hurt[2] die[4] gib[3]  = 23
rows:  E SE S SW W NW N NE
json:  {"anims":{"idle":{...,"offsetX":0},"walk":{...,"offsetX":32},
        "attack":{...,"offsetX":224},"cast":{...,"offsetX":320},
        "hurt":{...,"offsetX":448},"die":{...,"offsetX":512},
        "gib":{...,"offsetX":640}}}
```
`tools/atlaspack/bhpix.pack_atlas` writes this; `validate_atlas` lints it.
