# Dark Eden (1997 / relaunches, SOFTON) — the horror register

**Evidence:** director link `youtu.be/-UAVwmseC8o` ("Darkeden Awakening PVE+PVP",
174 s; contact sheet `darkeden-awakening-pvp-contact.jpg`, key frames
`-t0000s…-t0174s.jpg`, thumb `-thumb.jpg`), Steam *Cataclysm* manual (SOFTON,
1999–2014), mmos.com review, web stills (Perona/Lime interiors). All frames are
a modern private-server client; the sprite art is the 1997 originals, the FX
are later additions — treat FX density as **over-reference**, not target.

## Answers to the bible's §3.1 questions

**Vampire-vs-slayer asymmetry at tiny scale.** In the castle frames the two
sides read by *body language* before colour: slayers are upright, gun/sword
arms out, cloth-and-kevlar silhouettes; vampires are hunched, clawed,
wide-shouldered, and the transformed ones (bat/wolf) are non-human outlines
entirely. Race identity = **pose + outline**, never a palette swap. → §9 v0.2
Vampire note: the base body must permit a *second spine* (hunched idle,
forward-hanging arms); don't bake an upright idle into shared bones.

**Red / black / arterial palette.** The Dracula Castle interior (frames
t0034–t0104; measured cell-median 11–15, 74–78 % of pixels under luma 30,
mean saturation 8–11/255 — `qa/luma-audit.json`) is near-black stone with **red carpet runners and red-lit
windows** as the only saturated surfaces; blood pools and the "Cause Critical
Wounds"-class spells are the same arterial red pushed brighter. Everything else
(walls, floors, skin) sits under ~25 % saturation. Lesson we adopt: *one*
saturated hue per scene, and it must be the fiction's hue (blood). Ours is the
reserved ramp — arterial crimson on blood magic only.

**Gothic UI framing.** Stone-arch top bar, iron-riveted panel edges, a party
list with portrait busts (left), spell tooltip on black with grey serif text,
world clock ("1978/4/10 22:07:44") and a monster/chief **counter** panel. The
counter panel is a cheap drama widget (kills toward an objective) — adopt as
the bounty-board progress readout.

**Curse/magic light against a drained world.** Spell light is *additive* and
briefly enormous (the pillar in t0034: a white-gold column 3–4 tiles tall,
0.5 s). Because the world is drained, even a 6 px violet glyph reads. This is
the licence the bible grants us: violet `#6B4A8A` curses, arterial red, choir
gold — **never on terrain or mundane gear**. Verified in the style tile: the
only saturated pixels are the callout text and the firebolt.

**Blood-as-resource feedback.** Drain = a red beam from victim to vampire plus
a red number stream; blood pools persist as flat matte decals (frames t0068,
t0138 show pools outliving the corpses). → §4.4 "blood is matte, deepest red on
screen, 10 min" is DE-faithful.

**Enchanter buff telegraphing (Cultist reference).** Buffs are announced by a
ground ring + a short overhead glyph; the ring colour is the buff's identity
(green regen, blue protect). We keep the *ring + glyph* grammar but move the
colour language to the T-066 lock (heal green, bless gold, curse violet).

## Things NOT to copy

- Modern relaunch FX density (fireworks everywhere) — fails our crowd rule
  (FX occlude casters in t0104/t0138).
- Motorcycles/helicopters/SF layer (manual §A.3) — outside Vessalia's fiction.
- Brightly coloured hotbar icon backings — ours use the T-066 five-colour
  backing language only.

## Steal list → briefs

| DE element | Where it lands |
|---|---|
| hunched vs upright race silhouettes | §9 base-body spine note; Feral Ghoul / Hollow Hound hunch |
| arterial red as the *only* saturated hue | reserved ramp; Blood Bolt, Bloodfiend, Gravemother telegraphs |
| persistent matte blood decals | §12 gore set (6 decals × 3 decay) |
| ring + glyph buff telegraph | Bless/Ironskin/Haste/Sanctuary VFX briefs |
| kill-counter panel | bounty board UI widget |
| red-carpet-in-black-stone interiors | Drowned Crypt nave: one red runner to the bell |
