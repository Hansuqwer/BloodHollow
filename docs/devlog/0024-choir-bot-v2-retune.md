# 0024 — the choir finds its voice, the numbers move (Sprint 22)

## Choir-bot v2 (T-054b's second half)

The campaign bot profile grew the kit-v2 channels, potion priority and a
wider safe-chase band:

- **Chorus (ch6, Choir L9)**: fires only with a real party (the S13
  formation) and ≥2 voices inside the 6-tile sweep — a choir of one is a hum,
  and the bot knows it. Recast at ~2.5 min; the server refreshes, never
  stacks.
- **Mass Mend (ch7, Choir L12)**: fires when two or more roster rows are
  sub-60% in range — the 18 mp must beat two single-target mends — and then
  suppresses the single mend for that tick. Full-hp members are skipped
  server-side; the stitch never spills.
- **Haste (ch8, L10/L11)**: self rotation gear while a mob stands in swing
  range; 60 s buff, ~70 s refresh.
- All three gates read `content::kitSkillUnlock` from the shared table — the
  server enforces the same levels, so an early cast is a quiet no-op, never a
  gamble.
- **Potion priority**: baseline sip band stays <50%, but a mob in swing range
  or a live retreat raises it to <65% — flask before fangs.
- **Safe-chase band**: sub-50% with a mob adjacent breaks off (existing);
  new: sub-35% with a threat within 6 tiles breaks off too — the band has a
  floor now.
- SUMMARY/per-bot telemetry gained `choir c6/c7/c8` counters.

The channels unlock at L9-L12, above the campaign's L4-L6 plateau — this
leg of the profile is the *wiring*: correct gates, correct pacing, replay
clean. The casts light up once the retune below lets legs climb.

## T-034b: the plateau, levered

The duel table named the wall precisely: hounds 9/9 with blade+armor (fine),
gnolls and the widow **0/9 bare mid-kit** at every sampled player level.
Three levers, no economy touched: widow dmg 24→22, widow leash 12→10 (kite-
to-leash-break becomes real), gnoll dmg 18→17. Bare duel rows stay 0/9 on
purpose — the widow remains a gear-gate boss solo; the levers pay off in the
pack context where leash breaks, party play and choir support live. Card:
`docs/tasks/done/T-034b.md`; fresh table `logs/duel-table-s22.csv`.

## Contract

Journal epoch **5 → 6**: mob content shifts the sim under v5 journals; the
replay refuses them by contract (named diagnosis, not a lie). The refusal
fired for real during this sprint's own mid-flight smoke — the contract is
not decoration.

Also this sprint, prelude: **T-049x relog-launderer** (devlog 0023) — one
inv-blob grammar for live + replay, line-buffered journals, truncated-hash
refusal, fingerprint probes. The M2b-final chain's closing evidence is
committed against the fixed binary.

## Gates

- Suite **105/105** (327,925 assertions); ctest 2/2 (duel selftest included).
- Fresh S22 smoke leg (`tools/s22_smoke.sh`, 6 bots × 240 s, cadence 25):
  replay 0 mismatches on the epoch-6 build; choir counters in the SUMMARY.
- `tools/bh_probe_leg.sh` PROBE_VERDICT equal (T-049x evidence, devlog 0023).

## Next

A post-retune campaign chain (m2b rules, L8 target) to see whether the
gnoll band and the widow leash actually move the L4-L6 plateau — and, if
legs climb past L9, the choir's first real verses.
