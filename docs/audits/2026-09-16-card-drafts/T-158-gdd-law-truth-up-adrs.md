# T-158 — GDD law truth-up + ADRs for seven shipped deviations (P1, audit 9/20)

## Context
Audit phase C. Seven shipped laws contradict `docs/02-gdd.md` with no ADR, which
AGENTS.md forbids ("If the task conflicts with the GDD or an ADR, stop and flag
it"). None of these is necessarily *wrong* — several are better than the spec —
but the spec is the contract future agents read first.

| # | GDD says | Tree says | where |
|---|---|---|---|
| 1 | refine odds 100/90/80/65/50/35/25 | **100/100/60**/65/50/35/25 | `server/src/world.cpp:2153` |
| 2 | +4..+6 fail → −1 level; +7 fail → reset | **SHATTER at `refine==2` failure** | `world.cpp:2162-2173` |
| 3 | ore purity 1–10 + fodder tiers ±% | **absent** (part + 50 g toll) | grep 0 hits |
| 4 | gates 100 000 HP + siege-damage skills ×3 | **300 HP** + `/breach` ram | `server/src/world.h:368` |
| 5 | crown channel 60 s | **10 s** after a 60 s uncontested attune (attune is not in the GDD) | `world.h:373-374` |
| 6 | pledge creation CHA ≥20 + 100 000 g | **L ≥10 + 10 000 g** (flagged in code, no ADR) | `world.h:295`, `world.cpp:2994` |
| 7 | Blood Moon [v0.2]: spawn ×2, Pale Sow, +loot tier | shipped **inside MVP** as curse ×2 + night bite 130 % only | `world.cpp:1523-1550` |

Plus smaller drifts: mob XP tier multipliers (×3.9/×4.9 measured vs ×8/×20/×100
spec), leash 18 tiles vs 10–16 per row, glow tier 2 (+10) unreachable at a +7 cap,
no teleport-scroll sink, mob 1007 rename (D9 "Waxen Celebrant") unapplied.

## Scope
- Director decision per row: **adopt shipped** (amend the GDD) or **revert to
  spec** (file an implementation card). This card writes the decisions down; it
  does not silently pick.
- One ADR covering the enhancement/economy law changes (#1–#3) and one covering
  siege-law simplifications (#4–#5); #6 joins the stat-model ADR (T-160); #7 gets
  a change-control note per MVP §1 (cut something of equal cost, or defer the moon).
- Amend `docs/02-gdd.md` §7/§8/§9/§6/§11 with the shipped numbers + ADR refs.
- Update `README.md` §Status ("Next: Phase-4 pledges/siege spine" is stale; the
  `logs/t107.bwj` gate-leg citation now refuses at epoch 27), `AGENTS.md` DoD notes
  from T-154/T-155, `docs/tasks/README.md` (move T-138..T-145 to `done/`), and
  `docs/art/00-VERIFY.md` rows now shipped.
- OUT: implementing purity/fodder/100k gates/60 s crowns (separate cards if the
  director chooses "revert").

## Acceptance criteria
1. Every row above has a recorded decision + owner + (if adopted) the GDD diff.
2. Two ADRs filed and linked from the GDD sections they change.
3. `grep -n "Phase-4 pledges" README.md` → no hit; README's gate-leg citation
   points at a leg that replays at the current epoch.
4. No code, wire, schema or epoch change in this card.

## Evidence owed at merge
Decision table, ADR files, doc diffs, devlog, board row.
