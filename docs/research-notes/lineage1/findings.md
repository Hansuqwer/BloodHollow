# Lineage 1 (1998, NCSoft) + Remastered — the grade and the politics

**Evidence:** director links `youtu.be/ocDVnwvBOvI` ("THE VOID SERVER LINEAGE 1 |
4 VS WHOLE PLEDGE", 1342 s — classic-era private server, Tower of Insolence +
Giran-style town; contact `l1-void-4v-pledge-contact.jpg`, key frames
`-t0000s…-t1350s.jpg`) and `youtu.be/tJU0SoQo0-c` ("Lineage 1 Pet System Guide
… Remaster Classic", 785 s — Remastered client, Singing Island; contact
`l1-remaster-classic-pets-contact.jpg`). Plus web stills (2001-era Talking
Island, reddit r/Lineage), hiddenhosts.com Classic-vs-Remastered table.

## Answers to the bible's §3.2 questions

**The overall grade — this is OUR base grade.** Void-server frames: stone is a
warm-grey/olive (~#6a6656), grass a dead olive-brown, wood a cold brown; whites
are bone, never pure. Saturation stays under ~30 % everywhere except spell FX
and name tags. Blacks are *not* crushed — Tower floor cell-median 72–82/255 with only
2–4 % of pixels under luma 30 (`qa/luma-audit.json`), p90 109–124 (Δ 37–42).
Town frame t1080: median 66, p90 114 (Δ 48). The first-draft figures
("floor ≈ 95, sprites ≈ 150+") were eyeballed high — struck. **The Δ, not the
absolute floor, is what the R-LUMA gate (≥ 25) adopts.** Our earths are darker than L1's
(mud, rot) so we hit the gate by lifting the sprite palette, not the ground.

**Parchment / iron UI.** Bottom HP/MP bar with numeric readout inside a carved
iron-and-gold frame; top skill bar of square iron-framed icons with per-school
tint; parchment "Game Tip" scroll in the 2001 still; the Remastered client
keeps the same *layout* with flatter dark panels. → our chrome = HB layout with
L1 iron/parchment materials, Soma silver trim only on modal NPC dialogs (GDD §2).

**Alignment language.** Remastered frame t0155: the target's name tag is
plain white, hostile mob tag orange-red bar; classic L1 shows chaotic players
as **red names** and lawful as white, with the alignment word ("Neutral",
"Lawful") on the character sheet. Void frame t0000: "Immortal Lich" boss name in
red. So the era grammar is: **name tag colour = law/threat; sprite unchanged.**
Our 3-band set (T-057 already ships `karmaBand`; client colours lawful
`(170,200,255)`, neutral `(190,190,200)`, chaotic `(235,60,50)`) is
era-faithful; we add a 6×6 badge glyph left of the name for colour-blind
readability (§11).

**Siege crowd composition at gates.** Void frames t0270/t0540: fights happen
in **corridors defined by walls** (the Tower's stone partitions), 6–15 bodies,
each with a 3 px HP bar; the readable element at that density is the **HP bar
row + spell rings**, not the sprites. Gate fights in classic sieges are the
same: a wall line, a doorway, a pile. → Weeping Castle kit: gates are 2-tile
doorways in 28 px-high prism walls (matches `iso::drawPrism` height already
used), so piles form where the era formed them.

**Castle interior dressing.** Stone with weep-stains, torch sconces every ~4
tiles (warm pools), banner poles, a throne dais 1 step up. Identity by banner
colour only — masonry is faction-neutral. Adopted verbatim in §6.5.

**Remastered upscale — our cleanup tolerance limit.** Comparing t0155
(Remastered) to classic stills: Remastered *sharpened outlines and re-rendered
3D-derived sprites at ~1.5× pixel density*, kept the palette and the frame
counts. Community verdict (r/MMORPG 2025 thread): "blurriness was sacrificed
for being overly sharp… less thickness, harder to see where a character starts
and stops." That is exactly the failure mode the bible warns about: **sharpen
the silhouette, never thin it.** Rulebook rule R-OUTLINE: the 1 px dark outline
is *always* the outermost ring; never anti-alias it, never drop it on upscale.

## Things NOT to copy

- Remastered's flat translucent black UI panels (reads 2018, fails era test).
- Auto-hunt HUD widgets (PSS) — GDD §12 explicitly excludes.
- 3D-ish soft shading on Remastered monsters — our lock is HB hand-drawn.

## Steal list → briefs

| L1 element | Where it lands |
|---|---|
| warm-grey/olive stone, dead-olive grass, bone whites | terrain palette strips (town, castle) |
| floor luma ≈ 95 under sprites ≈ 150 | R-LUMA ≥ 25 gate |
| red/white/blue name tags, sprite unchanged | §11 karma bands (+ badge glyph) |
| corridor-and-doorway pile geometry | Weeping Castle gate kit |
| torch sconce every ~4 tiles | crypt/castle scatter: warm light pools |
| iron-framed HP/MP with numbers | core chrome |
