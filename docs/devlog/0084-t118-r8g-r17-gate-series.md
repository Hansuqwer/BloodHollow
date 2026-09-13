# 0084 — Ten legs at the gate: the r8g → r17 series, and the wall that stays

T-118 came back from a handover (`docs/handover/T-118-r8g.md`) with one
instruction: execute the continuation. The r8 series had left the party
looping at the crypt hatch; the prescribed fix was the smallest one — drop
the quorum wait at node (22,21), the double-leash pile where every full r8
leg had lost two or three of five. That fix became **r9**, and r9 did what
no leg had done: three of five bots walked into the Drowned Crypt Depths and
sighted the Gravemother (map5 entries at 692.9 / 692.5 / 617.1 s of 900,
replay clean). The clock died ten seconds later. No boss fight. The gate's
hard criterion — *reach the font* — was passed; its deliverable — the boss
verdict — was not.

What followed was the lesson. Nine more legs, each built on the forensic
corpses of the last:

- **r10** put live kits on the party (the disclosed class_id seed — the
  in-game oath rejects L13 characters) and reached map 5 at **131 s**, then
  death-spiralled at the Cantor lair: the runner's chase exemption sent the
  brain off-route after a mark and the column strung out behind it.
- **r11** killed the chase entirely and pinned the millers point-blank on
  the pile node — a 900 s knife-fight with swarm 9 that went nowhere.
- **r12** restored the point-blank swing-and-ride and made the pile node
  wait-immune; the ring-pull converged at (32,18) and the solo re-crosser
  died in the cocoon corridor. The journal showed something new: respawned
  bots oscillating between (14,14) and (30,14) on map 1 — the return march
  was broken.
- **r13** tried one brain per zone, stack-only hatch crossings, an unbroken
  march. The march out-sprinted its own kills; the racks were never culled,
  only collected, and the entire double-leash field converged on the first
  stop. Wiped in twenty-five seconds.
- **r14** went home: r9's exact posture, keeping only the hole fixes.
  Combat steadied (ten deaths per leg instead of fifty) — but the return
  march still pinned in sight of town, and r9's font reach did **not**
  reproduce. That settled the question the series kept asking: r9's entry
  was timing and luck, not posture.
- **r15/r16** rebuilt the march itself: a direction-aware runner election
  (the return march goes *west*; the old max-x rule had elected the
  rearmost bot as the column's brain), a 2-tile dead-band against
  position-staleness jitter, re-adoption after retreat drops. The first r16
  build froze the party at staging for a full 900 s — the re-adoption ran
  every tick and snapped routeIdx back the instant the machine advanced —
  an empty journal as the only evidence; r16b gated the snap to
  standing-on-a-node and the march finally moved.
- **r17** took the stable machine and doubled the budget to thirty minutes.
  Deepest reach: the cocoon corridor (x=24) and one death at (19,20) with
  swarm 13, one node short of the depths. Eighty-four deaths, zero map-5
  entries, replay clean (ticks 36 042, mismatches 0). Twice the time, same
  wall.

The wall is the finding. Every wave pays two or three of five to the
respawn-swarm convergence — racks refill every 30 s, the cocoon widows hold
the hatch exit, the barrow ring leashes onto the stairs — and the
respawn/remarch cycle compounds slower than any budget tried. Bot-side
iteration converged: posture stable, deaths per leg steady, the march works.
The remaining blocker is content-side geometry and respawn pacing, and no
bot tweak in this series changed it.

The battery never blinked through any of it: 209/209, 329 052 asserts,
duel pin `b273be661b54673a` untouched, every leg journal replayed
bit-exact. TEMP trace scaffolding came out at close-out. Card to
`docs/tasks/done/T-118-m3-gate-raider-series.md` with the verdict; full
forensics and the do-not-retry list in `docs/handover/T-118-r17.md`;
the r17 journal force-added as the journal of record (r9's font-reach
journal was overwritten by later legs before archiving — its [raid]
evidence survives in `logs/m3_gate_bots_r9.log`).

The gate is reached. The boss is still unmet. That is the honest shape of
it, and the next session starts from the handover, not from scratch.
