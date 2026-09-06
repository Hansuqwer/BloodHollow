# 0022 — the choir rounds out (Sprint 21): Chorus / Mass Mend / Haste

## What shipped (content half of T-054b)

Three new skill channels, because the Cultist's mixed-duel verdict needed
*levers*, not stat padding:

- **Chorus** (Pale Choir L9): the whole sworn circle catches the verse —
  +5% hit&dmg for 2 minutes. What the audit found is what stays law: a
  choir of one is a hum (party required), voices out of range don't sing,
  and the verse refreshes rather than stacking.
- **Mass Mend** (L12): everyone bleeds at once, so everyone heals at once —
  at the era's half-strength tax and never spilling over full vitality.
- **Haste** (Gravecaller L10 / Choir L11): the rotation gear the duel table
  asked for: swing cadence drops from 16 to 12 ticks for a minute.

New wire lanes: chorus sings over *each* member (kind 13, molten gold) and
haste streaks amber over the quickened (kind 14) — both ride the synth kit
(choir sting / steel swipe). Journal epoch stands at 5: the new commands
only write journal lines when someone has the channels.

## Deliberately deferred (integrity of the running gate)

The **choir-bot v2 profile** (potion-priority + safe-chase heuristics and
ch6/7/8 usage) and **T-034b number retuning** would swap bot/server
binaries mid-chain and poison M2b-final continuity — both land immediately
after the chain closes, each with its own fresh smoke leg.

## Gates

- Suite **98/98** (327,852 assertions) — 5 new T-054b cases pin the party
  law, both multipliers, the exact half-heal amount, the cadence ratio on
  a sampled swing-clock, and the level/unsworn gates.
- ctest 2/2 green; T-049's repro harness (tools/t49_repro.sh) remains the
  one-command canary if any future record-path change ships.

## Next

When the chain reports L8 (or exhausts at leg 12): choir-bot v2 + fresh
campaign smoke, then T-034b camp/leash retune against the duel table.
