# 0025 — the retune lands, the plateau stays (Sprint 22, post-chain)

## The rerun

`tools/m2b_rerun_chain.sh` re-ran the M2b-final pacing chain end-to-end on
the post-T-034b build (epoch 6, choir-bot v2 bots, fresh campaign DB). 12
legs, ~10.8k ticks each, every journal replays **0 mismatches**:

| leg | maxLevel | leg | maxLevel |
|---|---|---|---|
| 1 | L4 | 7 | L4 |
| 2 | L5 | 8 | L4 |
| 3 | L5 | 9 | L4 |
| 4 | L4 | 10 | L5 |
| 5 | L5 | 11 | L4 |
| 6 | L4 | 12 | L5 |

**TARGET L8 NOT reached; plateau L4-L5** — statistically the same band the
closed chain hit (L4-L6). The retune did **not** move it.

## Why the levers missed

The T-034b shortlist (widow dmg 24→22, widow leash 12→10, gnoll dmg 18→17)
targets the **L7-L9 band**. The duel table locates the actual wall lower:

- Feral Ghoul (L3): blade alone = 9/9 at player L2/L3 — no wall.
- **Hollow Hound (L5)**: at player L4, blade = 7/9, **blade+armor = 9/9**;
  at L5, fists 3/9 / shank 5/9 / blade 9/9. The wall is **gear, not numbers**.
- Gnoll (L7) / Widow (L9): the retuned rows — bots never climb there.

So the campaign stalls in the hound band because a 9-minute leg doesn't
reliably complete the blade+armor+skill gear-churn (anvil, vials, gold)
before the next portal hop. The widow/gnoll damage levers are correct for
their own rows but irrelevant to the band the chain actually lives in.

## What this changes

The retune is kept — it's honest for the gnoll/widow band and verified
replay-clean. But the L8-in-one-sitting goal needs a different lever: the
**gear-churn economics** of the L4-L5 band (vial/armor/anvil affordability,
portal pacing, or a targeted hound-band number), not another mob damage
nudge. That's a fresh card, not a re-run of T-034b.

## Evidence

- `logs/m2b_rerun_gate.log` + `logs/m2b_rerun_leg1..12.bwj` — 12/12 replay
  0 mismatches, end states L4-L5.
- `logs/duel-table-s22.csv` — hound/ghoul rows above.

Suite 105/105, ctest 2/2, epoch 6, chain binary unchanged from Sprint 22.
