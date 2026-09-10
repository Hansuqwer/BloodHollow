# 0072 — The choir has a Cantor (T-103)

T-103, content (epoch 16→17). Last of the three elites, and the only one that casts.

## What landed

- Row 1014, choir spawner, four tests, GDD elite line fully shipped (all three names now carry "shipped" marks — the §9 promise is kept).
- The `boss = 1` audit: exactly one consumer (the bolt branch), verified by grep across client/duel/engine, so the flag buys the instant bolt and nothing else. Telegraphs stay gated on 1009 by mobId, not by flag — a distinction the next boss card should preserve.
- One test-only ordering gap found live: `loadZone(3)` on a bare world throws (`spawnAnvils` assumes zone 1 booted first). Live boot always loads zone 1 first, so this is test-ordering, not a live bug — the test now mirrors boot order. Stated, not patched, because patching load order for a test-only path would be change without a patient.

## Judgment calls

- Half Mother's rate (52t), not a new cadence family: the bolt numbers already have a pin (T-070) and a night rule (T-061); a third cadence reuses both without new law. If the Cantor feels quiet at 52t, the lever is the number, not the mechanism.
- 4100 xp (5×, not 4×): the caster premium prices kiting risk — melee elites can be cornered, a bolt-caster cannot. One sentence of design, flagged for the director, derived from nothing because there was no precedent. Honestly: this is the single invented number in three elite cards.
- 60 minutes: the stagger completes (30/45/60). No two elites rotate together, ever, by construction.

## Soak (epoch-17 validation leg)

`logs/t103.bwj`: 14-bot grinder mix, 540 s → `[replay] OK ticks=12801 sessionCmds=7830 hashes=513 mismatches=0 entities=182`. Bots: campaign 2/L4, fighter 6, pilgrim 12, wander 112 — the wander count reads high until the two checks: killers spread L2/L3/L5/L7 with scattered grounds (no L15, no perch), and the leg total (132) sits mid-band (t094: 130, t101: 156, t102: 160). Roam-RNG per T-083, not a regression. The choir sings undisturbed (no crypt traffic, as stated).
