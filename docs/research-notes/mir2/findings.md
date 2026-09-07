# Legend of Mir 2 (2001, WeMade) — the grind texture

**Evidence:** no director link supplied; web captures archived as `mir2-web-1..8.jpg` (+ `mir2-web-contact.jpg`): LOMCN forum
"lom2-screenshots" 4-up (Bichon town square / castle wall line / green-poison
cave brawl / red mass-PvP), LOMCN "3D Models to Sprite Frames" thread (Crystal
client `.lib` libraries, per-image offsets), LOMCN minimap code (map cells store
`BackImage / MiddleImage / FrontImage` indices + `FrontAnimationFrame`), 01-research
§4 sources. The 2025 mobile "Mir2: Destiny" material is **not** a reference
(3D re-imagining).

## Answers to the bible's §3.4 questions

**Mob density composition — "wall of monsters".** The LOMCN 4-up bottom row is
the signature: 30–60 same-kind bodies in a 10×8-tile view, one palette family
(green Oma/poison cave; red Zuma/skeleton pile). Two things make it readable:
(1) the pack is **one silhouette repeated** — the eye counts *mass*, not
individuals; (2) the ground under a pack is *plainer* than elsewhere. → §6
scatter rule: spawner rects (`SpawnDef` w×h) get the **least** scatter; scatter
density ramps up *outside* them so packs read as clearings full of bodies.
Our spawner counts (`maxAlive` 6–10 per rect, e.g. `rats_east` 10, `bats_mouth`
10) are in Mir's low-density band; the "wall" feel needs 2–3 adjacent rects.

**What makes fields feel endless and hostile at tiny sprite scale.** Mir's
fields have **no horizon and no landmark repetition budget**: the same 3–4
ground plates continue past the AoI in every direction, punctuated by
single-tile scatter (stumps, rocks) with *no* clustering logic. Hostility is
mobs at the edge of the AoI walking toward you. Our AoI is ≤ 26 tiles
(README); the fields map is 64×48 — we get "endless" by (a) letterboxing the
map edge in soot, not a hard border, (b) a 1–2-tile fog band of darker cut
plate at the world rim, (c) `wanderRadius` 8–12 already in `kMobs`.

**Refine-gamble UI theatre — the anvil moment.** Mir's blacksmith dialog:
item slot centre, ore + accessory slots left, gold cost, a single "Upgrade"
button; the wait is a **timed silence** (item vanishes for real minutes in
classic Mir — you *come back* to learn the result), then either the item
returns with +N or the NPC says it broke. Modern servers compress it to a
progress bar + clang. Anticipation → toll → outcome. Our RFC-0001/T-041 anvil
already has: tier list, F to petition, kind-6 teal callout "Widow's Rite tier
N", destroy broadcast "the Anvil drank X's Shank". Missing for theatre:
**three distinct chrome states** (§11): *pending* (dim panel, ember glow
pulses), *success* (panel edge flashes choir-gold, item icon gets +N badge and,
from +5, the glow overlay), *degrade/destroy* (panel edge flashes arterial,
icon cracks in 2 frames then the slot is empty). Sound already exists (T-067
`toll`).

**Taoist support FX (Cultist's second ancestor).** Mir Taoist: yellow-paper
talisman throws, a green **poison cloud** DoT, a **soul-shield** bubble on
allies, summoned skeleton/白虎 pets, and "Healing" as a soft white sparkle.
Read: support FX are *paper-and-ink coloured* (yellow/white), small, and the
summon is the visual anchor. → Cultist: choir-gold replaces talisman yellow;
Raise Skeleton pet uses the ghoul-family bones; Mass Mend adds the ground
ring; Sanctuary = persistent hymn-glyph ring decal (the soul-shield idea
moved to the ground, per R-FX).

**Map/tile tech note.** Mir's cell = 48×32 px with three image layers; objects
are cut from big painted pictures into cell-sized slices ("turn parts of an
ordinary picture into objects you can go behind" — LOMCN). This is the same
paint-then-cut technique adopted in `cut_diamond()`; our objects (trees,
pillars) are single sprites with a feet anchor instead of slices, which the
painter-sort in `drawEntitiesOnline` already handles.

## Things NOT to copy

- Mir's bright primary spell colours (electric blue lightning, lime poison) on
  screen at once — fails palette audit; keep to the reserved ramp.
- Talisman yellow as a UI colour — reads Mir, not Vessalia.

## Steal list → briefs

| Mir element | Where it lands |
|---|---|
| repeated-silhouette packs on plain ground | spawner-rect scatter rule (§6) |
| edge-of-AoI approach as hostility | fields rim fog band; wander radii |
| anvil pending/success/destroy states | §11 anvil modal, three chromes |
| summon as the support's visual anchor | Raise Skeleton pet mini-sheet |
| paper-and-ink support colours → choir-gold | Cultist VFX briefs |
| paint-then-cut object layers | `cut_diamond()`, scatter sprites |
