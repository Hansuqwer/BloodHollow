# 0060 — The Mother announces her rot (T-091)

T-091, the run's first content-sim change (epoch 12→13) and its biggest card. The Gravemother finally fights like her GDD line says.

## What landed

- **Server:** wind-up state (3 transient fields, zero new RNG), fuse/strike/dodge/cancel semantics, bolt-mirror damage, shared-path preservation for the Gravecaller. Two real bugs caught by the new tests during the sprint: `killPlayer` cleared `attackTarget` but not the fuse (immortal wind-up striking corpse tiles), and the leash block returned before any clear could run (stale fuse walks home, then strikes a ghost on re-aggro). Both fixed: attackers' fuses die with the mark + think-top dud guard.
- **Client:** first caller of the T-ART-09 boss APIs — wind-up rings that stage themselves gold→orange→red against the server fuse, "!" floaters, "SLAM" callouts. No protocol change (kinds ride the existing `kind` u8).
- **Epoch 12→13** with the full liturgy: GDD line updated, fresh soak, old journal refuses exit 4.

## Judgment calls (flagged for director)

- **Telegraph tax:** the wind-up consumes the bolt cadence, so her DPS drops by roughly the fuse length per cast. That is the price of fairness and the reason telegraphs exist — but if she feels toothless in playtests, the lever is a shorter fuse (40t), not instant damage.
- **Gravecaller keeps the instant bolt:** only 1009 telegraphs. The shared cast path (curse lane, night bite) is untouched, so the L11 wall content behaves exactly as before.
- **Radius 2, holder-still:** both derive from readability (dodge with one sidestep-chain, see the boss plant her feet), not from any pinned number. If 2 feels small/large, it is one constant (`kSlamRadius`).

## Soak (epoch-13 validation leg)

`logs/t091.bwj`: 14-bot grinder mix, 540 s → `[replay] OK ticks=12801 sessionCmds=7482 hashes=513 mismatches=0 entities=360`. Bots: campaign 0/L4, fighter 54, pilgrim 28, wander 88 (mid-band; T-083 holds). Gravemother lives on map 5 — this map-1 leg exercises the regression half (bolt path for lesser bosses untouched, journals record cleanly under 13). Ghoul 37.2 s (n=32) noted under T-087 discipline and dismissed (no code path touches it).
