# 0075 — L9 logistics scoped: the tax is our own bot's belt (T-113)

Design card, no code. The question on the board since T-100: if faster L9
is wanted, which lever — vial stockpiling, armor, route?

## The answer the evidence gives

The server never capped belt depth. Marta's stock is a static list with no
depletion; BuyRequest takes qty up to the Blood Vial's stackMax of 16. The
30-trip tax in both climb legs is the **campaign bot's own policy**: a
4-deep belt refilled 2 vials at a time (`tools/bots/main.cpp`, the
"3-deep flask belt" branch). 40 vials drunk per pair per leg against an
8-vial pair capacity produces the observed stop count arithmetically.

So the cheapest lever is also the only free one: **T-114, bots v3 flask
belt** (depth 16, buy-to-depth at gold ≥ 240), then the T-090/T-100 repeat
discipline (same template, same start, 540 s × 2) with concrete verdict
criteria. No server change, no epoch.

## What is NOT the lever

- **Bone Plate** (def 6→11): ~1 hp/hit at widow raw 22 under
  `raw·100/(100+DEF)`; ~250–500 hp/leg ≈ 6–12 vials. Margin, not pace.
- **Stack 32 / safe road / forward vendor**: each costs an epoch bump or a
  content card and answers a constraint the belt fix removes first. The
  forward vendor reopens only if the re-run shows gold-starved belts (50 g
  start vs 480 g depth-16 fill).

## Judgment calls

- Scoped from existing telemetry only (T-074 discipline: no anecdote legs
  before the policy card lands); every number above cites T-090/T-100
  cards or code law.
- Kept the fence (Smuggled Vial) chaotic-only in every option: flattening
  the moral split to solve a bot pacing question would be the tail wagging
  the dog.
