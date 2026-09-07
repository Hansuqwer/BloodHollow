# Crowd test template (bible §15 "15-entity pile") — A6

**Tool:** `python3 tools/atlaspack/make_crowd_test.py` → `docs/research-notes/qa/crowd15_3x.png` + `crowd15_audit.json`.
Offline (bhscene). The in-client version of this test needs T-ART-01/04/05 and
is the sibling's to instrument — this page is the *proposal* for what it must show.

## Composition (fixed, so every batch is comparable)

| Slot | Entity | Tile | Tag | Notes |
|---|---|---|---|---|
| P1 | Ravager L7 (neutral) | (2.0, 3.0) | name + HP 70 % | casts Power-Swing callout |
| P2 | Cultist L6 (lawful blue) | (2.6, 3.4) | name + HP | cast ring decal under; Mass-Mend callout |
| P3 | Gravecaller L8 (**chaotic red**) | (1.6, 3.6) | name + HP | red must read first |
| P4 | Ravager L5 | (3.0, 2.6) | name + HP | |
| P5 | Cultist L7 | (2.2, 4.1) | name + HP | |
| M1–M8 | same-tier mobs | (3.6,2.2) (4.0,2.8) (3.4,3.2) (4.4,3.4) (3.8,3.9) (2.9,4.6) (4.8,2.4) (4.2,4.4) | 3 named, 5 unnamed; HP 50 % | 2 of 8 carry the rim (D1 comparison inside the pile) |
| FX1 | Firebolt frame 3 | from P1's hand | — | over entities |
| FX2 | cast ring 48×24 | under P2 | — | ground decal, never over |

Ground: the zone's plate (B0 fields plate for now). Views: day · engine night
02:00 · greyscale, 3× nearest.

## Gates (audit JSON)

| Gate | Rule | B0 stand-in result |
|---|---|---|
| occlusion | no entity > 50 % hidden by later-drawn entities | max 0.37 · 0 over 50 % — **pass** |
| callouts | ≤ 3 simultaneous, legible under night overlay | 2 — pass (7-px font stand-in) |
| FX | ≤ 40 % of caster body covered; ring under, bolt over | pass by construction |
| name tags | overlapping tag pairs | **7 pairs overlap** — see finding |
| silhouettes | rats count as individuals in grey | pass (8 low teardrops distinct) |
| chaotic priority | red name is the first tag the eye lands on | pass, but only because red is the *only* saturated text |

## Finding (new, from the first run)

The pile fails on **text**, not sprites. Five always-on player name tags in
~2 tiles collapse into an unreadable band at y−52 (7 overlapping pairs);
HP bars survive because they are 3 px tall and stack. Era answers:
Helbreath shows names on hover/target only (party names always); L1 always-on
with short names and a per-name plate. Proposal for the rulebook, **R-TEXT-2**:
*overhead names degrade to the 6×6 karma badge when more than 3 tags would
overlap in a 64×32 window; party and target names are exempt.* This is a
client rule (belongs with T-ART-07), recorded here for the director.

## What replaces the stand-ins

B3: M1–M8 become the real 1001–1005 sheets (S/SE walk f0) — the script takes
any 32×48 cells. B6: P1–P5 become the class bases (3 classes, both sexes
across the five slots). B8: FX1/FX2 become the real strips. Re-run, re-read
the audit, attach to the batch's LICENSES block.
