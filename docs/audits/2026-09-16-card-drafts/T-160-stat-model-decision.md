# T-160 — Stat model: six stats, or an ADR'd five (P1, audit 11/20)

## Context
Audit findings **B2.5 / C1**. GDD §3 specifies six stats (STR VIT DEX INT MAG
**CHA**) with CHA driving aura radius, pet slots, pledge creation (≥20) and vendor
prices. The tree has `str/vit/dex` assignable (`World::assignStat`,
`server/src/world.cpp:338-344`, `stat > 2` refused), `intg/mag` seeded by the kit
(`shared/content/kits.h`) and never assignable, and **no CHA at all** — the code
says so itself: *"GDD §8 gates creation on CHA >= 20; the shipped stat model is
str/vit/dex + kit-derived int/mag with no CHA"* (`world.cpp:2994-2995`), and the
pledge gate was re-cut to L ≥10 + 10 000 g because of it (`world.h:295`).
`messages.md` also still says "INT/MAG land with casters in a later phase" while
casters shipped.

## Scope
- Director decision, then implement exactly one:
  - **(A) Six stats**: add CHA (+ assignability for INT/MAG), migrate existing
    characters (unspent points only — never rewrite history), wire `OwnStats` +
    `StatAssign` for 0–5, hook CHA into aura radius / pledge gate / vendor prices,
    schema bump, epoch bump if drop/sim semantics move.
  - **(B) Five stats (ADR the cut)**: keep STR VIT DEX INT MAG, make INT/MAG
    assignable (casters exist), hard-code aura radius 12 and the pledge gate, and
    strike CHA from GDD §3/§8 with an ADR + change-control note (MVP §1).
- Either way: client stat panel shows every assignable stat with its effect text,
  and `messages.md` loses the stale "until casters exist" line.
- OUT: respec NPC (GDD says v0.2), pet slots (no pets ship).

## Acceptance criteria
1. ADR filed naming the chosen model and the migration rule.
2. Every assignable stat is spendable in-client (hotkey or panel) and reflected in
   `OwnStats`; unit pin per stat.
3. Migration pin: a pre-existing character row (schema v14 fixture) logs in with
   the right stats and unspent points; no XP/level/gold change.
4. Aura radius / pledge gate / vendor prices read from the chosen model, with pins.
5. Replay `mm=0`; epoch/schema bumps stated and legged once (coordinate T-151).

## Tests required
Stat-effect pins per stat, migration fixture test, blob/persist round-trip.

## Evidence owed at merge
ADR, suite count, migration proof, screenshots of the stat panel, devlog, board row.
