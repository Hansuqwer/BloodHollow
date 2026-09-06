# 0026 — the wall is a treadmill, not a number (T-034c)

T-034c asked *why* the campaign plateaus at L4-L5 after T-034b, and what
actually moves it. The answer, from the rerun numbers and the duel table, is
that the wall is the **death-tax treadmill in the hound band** — and the
lever is a bot-profile gap, not another mob number.

## The ledger

A rerun leg is ~10.8k ticks (9 min), 2 bots, 114–141 kills, **24–48
deaths**. Kills gross ~11–14k XP (ghoul spine, ~90–110 XP each). Death tax
is 11% of the bar at L4 (143 XP) and 12% at L5 (235 XP), so ~40 deaths wipe
~7.6k XP — the same order as the kills earn. At L5 the treadmill is
break-even; the campaign sits there forever, which is exactly the observed
plateau.

## Where the deaths come from

The duel table already said it, T-034b just aimed at the wrong band. At L4
the Pit Blade beats a hound 7/9 but ends at **30% hp**; the hound marsh is a
pack camp, so the second hound kills a 30%-hp bot. Hide Armor (def 6) is
what flips 7/9 → 9/9 and lifts end-hp to 0.46. So the bot dies in the window
between *blade* and *blade+armor* — the gear-gate tutorial working as
designed, against a profile that doesn't complete the purchase on time.

## Why the gear never lands on time

380g of gear (blade 260 + armor 120) is affordable in a couple of minutes of
clean farming. But the campaign profile has **no re-gear trip**: every shop
branch is gated on `nearTown`, and the only home-bound path is the pilgrim
anvil walk. A campaigner touches town only at leg start and on
death-respawn. So the shopping is *coupled to dying* — the bot grinds,
dies, respawns, shops, walks back, dies. The gear-churn economics stall
because the churn is hitched to the death cycle.

## The lever

T-034d should add a **campaign re-gear trip**: walk home when gold clears
the next gear tier (blade @260g, then armor @120g), then back out. Bot-only,
no content numbers move, replay stays bit-exact (pathing is
client-predicted, server-validated). If that alone doesn't clear L6, the
cheap follow-on is Hide Armor 120→80. Touching the hound or the death tax is
a content-pillar decision, not this card's.

## Evidence

- `logs/duel-table-s22.csv`, `logs/m2b_rerun_*.log`, devlog 0025.
- Card: `docs/tasks/done/T-034c.md`.
