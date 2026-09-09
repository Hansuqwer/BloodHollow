# 0054 — The pilgrims are not broken; the toll is a mountain (T-086)

T-086, audit-only. `anvilTries=0` survived six more legs and looked like a bot bug. It is not.

## The one table that matters

Tier-1 wants 30 Rat Pelts. The richest pilgrim leg ever banked 5. Rats pay pelts at 40% and pilgrims stop farming rats ~60 s in. That is the whole probe: **the rite's first step costs ~6 legs of pelt income inside a one-leg harness.** Skill 20 and 120g are further gates behind it, and the blade itself takes half the leg to afford (50g start, 110g shank, `dbgNoBlade` ≈ 2100 samples/leg).

## Judgment calls

- No bot change: any "fix" that makes bots attempt the anvil in-leg would have to cheat the economy (conjure pelts) or lobotomize the route (farm rats forever and never level) — both worse than the telemetry gap. The counters (`dbgNoBlade/dbgNoGold/dbgNoPelts/dbgNoAnvil/dbgReady`) already discriminate every failure mode; they read 2106/2/48/0/0, which is exactly "poor, unequipped, and nowhere near ready".
- If the director wants pilgrims grafting in-soak, the lever is the Tier-1 toll (fewer pelts / lower skill), which is economy design with an epoch bump — flagged, not taken.
- Alternative read considered and rejected: multi-leg pelt banking would prove the rite end-to-end, but the soak harness is fresh-DB by contract (devlog 0030's false death-loop lesson). A persistent-progression rite demo needs its own card and its own DB discipline.
