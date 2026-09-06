# Task board

Card format: Context / Scope / Acceptance criteria / Tests required / Out of scope.
One card = one agent session = one PR. Move to `done/` with evidence pasted.

## Done — Sprint 1 (2026-09-04)

| Card | Title |
|---|---|
| [T-001](done/T-001.md) | Repo + CMake + presets + CI + warnings discipline |
| [T-002](done/T-002.md) | Window/input + fixed 20 Hz tick stepper |
| [T-003](done/T-003.md) | Isometric renderer (2:1 diamonds, prisms, painter sort) |
| [T-004](done/T-004.md) | Atlas/anim loader + procedural placeholder hero |
| [T-005](done/T-005.md) | Tiled JSON -> .bhmap converter + validator |
| [T-006](done/T-006.md) | Sim grid + deterministic RNG + A* |

## Done — Sprint 2 (2026-09-04, accelerated)

| Card | Title |
|---|---|
| [T-007](done/T-007.md) | Camera rig: follow/free-pan/zoom-at-cursor, bounds clamp |
| [T-008](done/T-008.md) | Day/night: real game clock (4h day) driving tint |
| [T-009](done/T-009.md) | Replay journal v0 + hash oracle (record/replay/verify) |
| [T-010](done/T-010.md) | Thornwall pass 2 (generator): graveyard, stalls, mud, grass variation |
| [T-011](done/T-011.md) | ADR-001..008 formal write-ups in docs/adr/ |
| [T-012](done/T-012.md) | Demo automation: tick-latched script, deterministic captures |

## Done — Sprints 3-4 (2026-09-04, accelerated): Phase 1 Netcore, M1 gate

| Card | Title |
|---|---|
| [T-013](done/T-013.md) | ENet transport + protocol v0 (messages.md -> generated serializers) |
| [T-014](done/T-014.md) | Login stub + account/char persistence (SQLite v1, ADR-0009 stub auth) |
| [T-015](done/T-015.md) | Zone server: tick loop + entity store + spatial hash |
| [T-016](done/T-016.md) | Authoritative movement sync + AoI deltas + client interpolation |
| [T-017](done/T-017.md) | Chat (say/global/system) + spawn-diff reconnect view |
| [T-018](done/T-018.md) | tools/bots v1: 20 headless wanderers + 10x60s soak gate (M1) |

## Done — Sprint 6 (2026-09-04, accelerated): economy & death (Phase 2)

| Card | Title |
|---|---|
| [T-021](done/T-021.md) | Inventory/equip/loot: item table, gear in combat math, mob drops |
| [T-025](done/T-025.md) | Death: XP debt 10->25% of bar, de-level at 0 XP |
| [T-026](done/T-026.md) | Skills v1: Power Swing + Soma weapon-skill spine |
| [T-027](done/T-027.md) | Vendor v0: Marta, 5-stock shop, junk pawn, proximity trades |
| [T-028](done/T-028.md) | Bots v1.5: vial economy + power swings |

## Done — Sprint 7 (2026-09-04): trade, density, balancer, replay (Phase 2 finish)

| Card | Title |
|---|---|
| [T-029](done/T-029.md) | Trade window: commit-time validation, autosafe rollback (ADR-0011) |
| [T-030](done/T-030.md) | Fields content to L11 (7 mob kinds, 10 spawners), density rule |
| [T-031](done/T-031.md) | Bots v2 grinder economy + server TTK balancer feed |
| [T-032](done/T-032.md) | World journal record/replay; wipe reproduces bit-exact |

## Open — M2 gate pass checklist (de-QA)

| Card | Title | Notes |
|---|---|---|
| T-033 | M2 gate run: 620s soak w/ grinder mix, balance bands checked, wipe replay attached, human trade pass | gate=of record + devlog |

## Done — Sprint 8 (2026-09-04): zones-in-process (Phase 3 launch)

| Card | Title |
|---|---|
| [T-035](done/T-035.md) | Thornwall Crypt map: gen+conv, L12 Revenant Sexton, sibling portal |
| [T-036](done/T-036.md) | Zones-in-process: N zones in World, portals, per-zone AoI/sim |
| [T-037](done/T-037.md) | Client zone handoff via Welcome reuse; map hot-swap |
| [T-039](done/T-039.md) | persist schema v4: characters.map_id zone persistence |

## Done — Sprint 9 (2026-09-05): anvil/aura spine (RFC 0001)

| Card | Title |
|---|---|
| [T-041](done/T-041.md) | Widow Anvil furniture + tryAnvil gate chain |
| [T-042](done/T-042.md) | Aura tiers I–II live effects (+atk, regen) |
| [T-043](done/T-043.md) | Protocol v31→131, inv blob v5, karma v6, bless journalling |
| [T-045](done/T-045.md) | Pilgrim bot profile (rite economy + anvilTries telemetry) |
| [T-046](done/T-046.md) | Karma columns + moral split (+15% XP / +15% gold) |
| [T-038](done/T-038.md) | RFC 0001: anvil/aura spine (implemented by T-041..T-046) |
| [T-040](done/T-040.md) | bot deaths probe: CombatEvent-driven (carried into S8.5 gate) |

## Done — Sprint 10 (2026-09-05): proc riddle + panel (aura spine complete)

| Card | Title |
|---|---|
| [T-047](done/T-047.md) | wireKind furniture floor assert + aura procs III–V (cleave/sunder/graft) |
| [T-048](done/T-048.md) | client anvil panel, karma readout, aura rows, kind-6/7 floaters |

## Done — Sprint 11 (2026-09-05): replay determinism (M2b unblocked)

| Card | Title |
|---|---|
| [T-049](done/T-049.md) | record-path determinism: shared applyWorldCommand; replay 3×300s clean |

## Done — Sprint 12 (2026-09-05): duel lab + campaign gate evidence

| Card | Title |
|---|---|
| [T-034](done/T-034.md) | bh_duel: gear-aware offline duel harness + era table |
| M2b (partial) | wipe-replay ACROSS chains bit-exact (12 legs, 1139 hashes); L1->8 campaign measured, plateau=L6, stays Phase-3 xparty |

## Done — Sprint 13 (2026-09-05): party system (Phase 3 spine)

| Card | Title |
|---|---|
| [T-050](done/T-050.md) | party core: wire v237, rules, slash verbs, despawn sweep, replay-invariant |
| [T-051](done/T-051.md) | party XP share: 12-tile radius, +12%/sharer, loot stays with killer |
| [T-052](done/T-052.md) | client party frame (roster panel, leader star, live hp bars) |
| latent fix | command-path WorldEvents drained pre-tick (anvil-floater-class bug) |

## Done — Sprint 14 (2026-09-05): class kits, Cultist-first

| Card | Title |
|---|---|
| [T-053](done/T-053.md) | class kits foundation: classId, persist v7, /kit oath, skill dispatch, MP pool |
| [T-054](done/T-054.md) | Cultist kit v1 (Mend/Bless/Ironskin) + Gravecaller Firebolt starter |
| [T-055](done/T-055.md) | race-proofed choir bot + dual-kit gate leg (mixed verdict, levers listed) |

## Done — Sprint 15 (2026-09-05): alignment & PK teeth

| Card | Title |
|---|---|
| [T-056](done/T-056.md) | PK law + chaos penalties: karma −(300+20d), whitening, drops, gallows, refusals, duels |
| [T-057](done/T-057.md) | alignment chrome: karmaBand wire, red nameplates, sheet stakes |
| correction | T-046 moral-split gate raised to GDD lawful >500 (xor-drift fix) |
| tooling | journal epoch marker `v N`; replay refuses mismatched epochs (exit 4) |

Execution prompt for the whole remainder queue: docs/prompts/phase3-remainder.md

## Done — Sprint 16 (2026-09-05): anvil gear-churn

| Card | Title |
|---|---|
| [T-058](done/T-058.md) | Durability: burn on swing/hit, dormancy at 0, `/repair` at Marta (persist v8) |
| [T-059](done/T-059.md) | Affixes v1: Whet / Warding / Leech rolled at gear-drop time (persist v9, journal epoch 4) |
| [T-060](done/T-060.md) | Refine at the Widow Anvil: parts+50g toll, 0→1/1→2 sure, 2→3 60% or SHATTER (persist v10) |

Gate: suite 83/83 (327,466 assertions); epoch-4 smoke replay OK
(ticks=1455 hashes=14 mismatches=0; bots mend=14). Devlog 0017.

## Done — Sprint 17 (2026-09-05): day/night stakes

| Card | Title |
|---|---|
| [T-061](done/T-061.md) | Nightcreep: mobs ×1.15 dmg & +1 aggro 21:00–05:00, paired-world bites pinned |
| [T-062](done/T-062.md) | Night economy: +10% XP / +25% relative drops after dark; overlay alpha floor 150 documented |

Gate: suite 87/87 (327,479 assertions); S17 smoke replay OK
(ticks=1292 hashes=12 mismatches=0; journal epoch stays 4 — zero new rng
draws). Devlog 0018.

## Done — Sprint 18 (2026-09-05): content drop

| Card | Title |
|---|---|
| [T-063](done/T-063.md) | Bonehowl Mine (mapId 4) + Drowned Crypt Depths (mapId 5), portals + validators |
| [T-064](done/T-064.md) | Gravemother L14 (xp×20) + Sepulcher Elites (xp×8, 8 camps) + Blood Bolt (+25% night pin) |
| [T-065](done/T-065.md) | Wanted Board (wire 66): session-scoped quarry cycle, payout over natural band |

Gate: suite 91/91 (327,815 assertions); S18 smoke replay OK
(ticks=1255 hashes=12 mismatches=0; epoch 5 — zones enter worldHash,
entities 291→579). Devlog 0019. **M2b-final L1→8 chain now running.**

## Done — Sprint 19 (2026-09-05): VFX + audio pass 1

| Card | Title |
|---|---|
| [T-066](done/T-066.md) | Callout sweep: bless gold / ironskin steel / party shimmer + petrify-fade vestiges |
| [T-067](done/T-067.md) | Procedural synth kit (7 voices, ~78 KB, BH_NO_AUDIO-safe) |

Gate: suite 91/91 (327,815 assertions); S19 smoke replay OK
(ticks=1182 hashes=11 mismatches=0; epoch 5). Devlog 0020.

**Phase-3 queue drained.**

## Done — Sprint 20 (2026-09-06): replay trust

| Card | Title |
|---|---|
| [T-049](done/T-049.md) | record-path determinism CLOSED: 3× cadence-25 6-bot soaks clean, burst-semantics pins, ctest working-dir fix |

Gate: 93/93 suite; t49_repro verdict fail=0; ctest 100%. Devlog 0021.
M2b-final chain resumed (leg-4+) after the sandbox-eviction restart.

## Done — Sprint 21 (2026-09-06): kit v2 content half

| Card | Title |
|---|---|
| [T-054b](done/T-054b.md) | Chorus (ch6, L9) / Mass Mend (ch7, L12) / Haste (ch8, L10-11) — party law, cadence, gates pinned |

Gate: suite 98/98 (327,852 assertions); ctest 2/2; epoch stays 5. Devlog 0022.
Deferred on purpose: choir-bot v2 profile + T-034b retune → after the
M2b-final chain closes (binary-swap would poison pacing evidence).

## Closed — T-049x (2026-09-06): the relog launderer

The M2b-final chain closed; its persist-round replay leg exposed a launderer:
live login and replay applyLogin parsed 7-field inventory blobs differently
(equipped dropped / dormant gear resurrected / affix-refine lost / slot order
scrambled), below `worldHash`'s id/zone/pos/hp coverage. One shared grammar
(`parseInvBlob`) + canonical logout save + `BH_DUMP_ENTS` fingerprint probes
+ `tools/bh_probe_leg.sh`. Suite 105/105; all journals replay 0 mismatches.
Devlog 0023. Epoch stays 5.

## Open — Phase 3 remaining

| Card | Title | Notes |
|---|---|---|
| M2b-final | L8-in-one-sitting pacing rerun (S13 party XP exists; measure uplift) | evidence: logs/s13_party_leg.*, devlog 0014 |
| T-034b | balance retune from duel table: camp leuk-for-leash levers | logs/duel-table-s12.csv |
| T-054b | Cultist v2: Chorus (party-wide), Mass Mend, Haste; choir-bot v2 (potion priority, safe-chase) | devlog 0015 |
