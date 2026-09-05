# RFC 0001 — the Anvil & Aura spine (Soma adoption, Phase 3 core loop)

**Status**: proposed (S8) · **Implementation cards**: S9: T-041..T-045
**Research basis**: Soma manual/wiki — weapon skill rises by use (+1 atk per
20 skill); aura tiers quested at the Anvil priestesses (twin NPCs), paid in
monster parts + gold; top tiers risk item death; first attempts guaranteed.
Private-server lesson already folded into our render: nights stay legible.
Helbreath lesson for callouts: red-caps stickers (live since S6).

## Why this system

Bloodhollow's long-run question is "why does my sword still matter at level
15?". Soma's answer — weapons grow **by use**, then **bleed to transcend** —
bundles tension, sink, and chase into one loop, and every tier feeds the
economy: monster parts out, gold in, gold reminted into gear at the vendor.

## Systems map (what exists vs what lands)

| piece | status | where |
|---|---|---|
| weapon skill by use (+1/25 hits, +1 atk/20) | **live (S6)** | `world.cpp trySwing` |
| vendor + gold economy + junk pawn 40% | **live (S6)** | `world.cpp vendor*` |
| monster parts (4001..4005 junk tiers) | **live (S6-S8)** | `content/items.h` |
| anvil objects + `AnvilOp` wire msg | S9 (T-041) | zones 1 & 3 furniture |
| aura tiers I..V quests | S9-S10 (T-042) | tier table below |
| item flags persistence (auraTier, family) | S9 (T-043) | schema v5 |
| aura callout/fx + fail sticker UX | S10 (T-044) | client fx queue |
| balance + anvil telemetry | S9 (T-045) | `[aura] tier=N res=...` |

## Anvils (world furniture)

- *Widow Anvil* — town plaza, zone 1 (Marta's footprint; she taps the parts
  price). Services tiers **I–II**.
- *Bone Barrow Anvil* — crypt sanctum, zone 3 (guarded by the Sexton's room).
  Services tiers **III–V**.

Stand within 2 tiles; **F** opens the anvil panel (vendor UI submode). Wire:
new C2S `AnvilOp{tier}` behind proximity checks; server validates skill gate,
parts in inventory, gold floor — atomically, vendor-style.

## Tier table (sword family; others clone the shape)

| tier | req skill | cost | effect | fail |
|---|---|---|---|---|
| I `Edge Rite` | 20 | 30 Rat Pelt + 120g | +3 attack | — |
| II `Blood Wed` | 50 | 15 Ghoul Finger + 400g | +2% hp per 60 ticks | — |
| III `Triple Fang` | 80 | 10 Widow Silk + 1.2k g | proc: hit 3 targets | item −2 levels |
| IV `Sovereign Cut` | 120 | 5 Revenant Ash + 4k g | +35 power strike, always hits | 50% item destroyed |
| V `Crimson Pact` | 150 | 3 Revenant Ash + 8k g | lifesteal 15% | 35% item destroyed |

First attempt at each tier **always succeeds** (Soma softened: one mercy, not
two — our economy is thinner). Item *levels* come with T-041's affix pass in
Sprint 9; until then tier III failure costs −20% of the item's gold value
(eaten at Marta).

## Karma tie-in (moral economy, two-sided)

- **Wiise (good):** tier III+ fail odds halved.
- **Kin (bad):** anvil gets +15% gold yield per transaction history, fail tables untouched.

Neither side is "the good one"; both are viable coin styles. The two-sided
incentive is the adoption's stated purpose.

## KPIs / verification gates (S9-S11)

- Part→tier pipeline observable in the balancer: `[aura]` lines + weekly count
  of IV/V successes per 100 attempts.
- Bots v3 "anvil-pilgrim" profile: farm parts, ride the ranks, report tier/hour.
- Never a quiet fail: destroyed-item events broadcast a chat line in zone +
  feed row (`the Anvil drank watcher_02's Shank.`).

## Open questions for the S9 plan

1. Weapon *families*: sword/axe/mace/2h auras independently tracked? (lean yes).
2. Should tier V VIse the XP-debt system's ledger on destroy (salt the wound)?
3. Visual: lifesteal particle + fail clang — needs client fx queue lane.
