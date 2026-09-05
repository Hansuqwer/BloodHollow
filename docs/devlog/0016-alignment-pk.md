# 0016 — Red hands, red names

Sprint 15, 2026-09-05. Cards: T-056 (PK law & chaos penalties), T-057
(alignment chrome). Source: docs/prompts/phase3-remainder.md, item 1.

## The semantics that bit

T-046's moral split fired at `karma > 0` — fine in isolation, treacherous the
moment whitening (+1 per at-level kill, +20/hr online) exists: one rat past
zero and every future kill paid the *lawful* carrot. The GDD always said
lawful **>500**; karma 5's +15% XP was a typo hardened into a test pin. Both
fixed in-line: pin now reads "5 earns nothing, 501 earns". This is why era
games write the rulebook down.

Whaling tax follows GDD §5 to the letter: `−(300 + 20 × level-deficit)`,
clamp ±1000, red prey is free (the gallows-market rule — hunting Chaos is
lawful sport), duel consent voids everything and *closes the duel on the
kill* — one death per handshake, no shield-camping a corpse.

## Teeth (chaotic deaths)

1–6 inventory items picked by the crowd (destroyed; ground-scatter is a later
card, and the card file says why), 15% per equipped slot on top, respawn at a
walkable-searched gallows tile instead of the temple square, Marta refuses
red coin on both verbs with a sneer, and the crowd hears it on ch-255 when a
soul blackens *or* whitens.

## Tooling dividend

The sim-semantics change (whitening) invalidated old journals by definition —
except it didn't, because karma lives outside `worldHash` until the band
matters, and both S13 and S14 legs still replay byte-for-byte. Not trusting
luck twice: journals now open with a `v N` epoch line and the replay runner
refuses mismatched epochs with exit 4 and a plain sentence instead of
noise-mismatching into a debugging evening. Record fresh gate legs after
sim-semantic sprints; old legs stay as history, not oracles.

## Gate

- Suite: **76/76** (7 new alignment cases), ctest 2/2.
- Epoch-3 smoke leg: campaign bots, party formation race-proof, 1327 ticks,
  13 hashes, **0 mismatches** on replay (logs/s15_smoke.*).
- Legacy S13/S14 journals replay green *still* (hash-transparency of karma
  below the lawful bar — measured, not assumed).

## Next (prompt continues)

S16 anvil gear-churn (durability/affixes/refine — the gold sink that feeds
chaos by accident), then S17 day/night (nightpack boosts under the locked
tint floor), S18 content drop, S19 VFX/audio pass 1. M2b-final fires after
S18, when the world has enough teeth in it to be worth timing.
