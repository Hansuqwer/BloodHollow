# 0049 — Wander drift: the spike was roam-RNG (T-083)

T-083, read-only. The extended-shift watch (devlog 0044: 24,16,24,66,68,84,146) looked monotonic and the entity-growth suspect (330→378) was credible. Two fresh short legs break it.

## Legs (240 s bots / 300 s soak, 14-bot grinder mix, epoch 12)

| leg | journal | replay | wander deaths | end entities | campaign / fighter / pilgrim deaths |
|---|---|---|---|---|---|
| A | `logs/t083a.bwj` | OK 6001/3184/241 mm=0 | **16** | 349 | 0 / 30 / 18 |
| B | `logs/t083b.bwj` | OK 6001/3167/241 mm=0 | **24** | 344 | 0 / 44 / 26 |

Ports 7895/7896, fresh DBs, `BH_HASH_CADENCE=25`. No code touched.

## Why noise, not a leak

1. **Mean reversion.** Full series with the three S33–S35 legs: 146 → 70 → 86 → 56 → 16 → 24. The "monotonic" run was 24→146 across the shift boundary; the next five legs fall back to the early baseline. A leak does not self-heal across identical harnesses.
2. **Spread killers, scattered grounds.** Leg A wander: L5:14 L3:2, five distinct lastDeath tiles (south road, marsh, bank). Leg B: L3:6 L5:12 L2:6, five more distinct tiles (west road, orchard, river). Zero L15 in both, zero perch-cluster (contrast T-074 legs 3/5: ALL L3 at one waypoint). This is the roam-RNG signature from devlog 0044, now at low amplitude.
3. **Same-harness variance dominates.** Fighter 30→44 and pilgrim 18→26 across legs A→B (identical regime, same binaries) — the fighting profiles wobble ±50% leg to leg. Wander 16→24 sits inside that band.
4. **Entity suspect: consistent but small.** 367 (t079) → 356 (t082) → 349 → 344 tracks deaths downward, so coverage direction holds — but a ~6% entity swing cannot explain a ~9× death swing (146→16). It stays a second-order watch, not the cause.

## Judgment calls / pins

- Short-soak (240/300) is sufficient for this watch: wander deaths accumulate from t=0 (L1 bots roam immediately), and both replays are clean. Full 540 s legs reserved for content-sim cards.
- No T-084. Reopen rule: a full-length 540 s leg with >100 wander deaths AND (perch clustering OR L15 involvement). Otherwise the watch closes next shift.
- `anvilTries=0` across all 14 bots in both legs (again) — owned by the pilgrim probe (task 6), not this card.

## Files

No code. Card `docs/tasks/done/T-083.md`, journals `logs/t083a.bwj` + `logs/t083b.bwj` (`git add -f`), server/bot logs alongside. Suite 160/160 untouched.
