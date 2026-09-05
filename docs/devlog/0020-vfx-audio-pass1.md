# 0020 — the game can now speak (Sprint 19): callout sweep + procedural audio

## What shipped

**T-066 callout sweep.** Audited the full floater lane table:

| kind | lane | look |
|---|---|---|
| 0 | miss | grey |
| 1 | hit | bone |
| 2 | crit | red-cap "!" |
| 3 | SLAIN | gold |
| 4 | xp/heal | green +N |
| 5 | Power-Swing | RED-CAPS over caster |
| 6 | anvil rite | teal tier ring |
| 7 | aura bands | cleave/sunder/graft tri-color |
| 8 | Mend | "mend +N" over recipient |
| 9 | BLOOD BOLT | crimson-violet, caster |
| 10 | BLESS | gold, target |
| 11 | IRONSKIN | steel-white, target |
| 12 | party share | warm gold xp shimmer |

Three of those (10–12) didn't *exist* as events before — the server now
emits them; CombatEvent wire is kind/amount-carried field growth, version
unbumped (237 still). Death got its petrify-fade: a 0.6 s sinking grey
silhouette at the departure pose, client-side vestige queue (16 cap),
furniture never ghosts.

**T-067 audio pass 1.** `synthkit.h` bakes seven voices at boot — swing /
hit / bolt / toll / choir / chime / death — out of sine sweeps, inharmonic
partials and the same 64-bit LCG family the map generators use. ~78 KB of
samples, zero files, zero network. `BH_NO_AUDIO=1` keeps CI and headless
runs silent-by-choice. Bolt binds to kind 9, mend/bless to the choir
sting, iron-skin and anvil to the toll, crit to the thud, SLAIN to the
low rumble that pairs with the vestige fade.

## Gates

- Suite **91/91** (327,815 assertions) after the event-shape additions —
  T-051/T-054 event-sensitive tests stayed green (the party-share shimmer
  roads, but earlier assertions count scoping, not totals).
- S19 smoke: replay **bit-exact** `ticks=1182 hashes=11 mismatches=0
  entities=575` — epoch 5 stands (client-side only for visuals; floater
  events consume no rng).
- Journal/bot "mend" counter on the S19 leg: `mend=13` and the choir line
  rang. Presence confirmed via telemetry even without a display.

## Screenshots (deferred to a display-bearing host — sandbox is headless)

```sh
# on a Linux/macOS desktop build:
./build/client/bh_client --online --server 127.0.0.1 --port 7819 \
    --name shot --pass shot --screenshot shots/s19
# scripted demo captures produce shots/s19-day.png & s19-night.png at
# the scripted marks; BH_NO_AUDIO=1 mirrors CI volume in captures.
```

## The queue is drained — what's left

Sprints 15→19 all landed consecutively in one session chain (fb9e514 →
head). Remaining open cards on the board: **T-049** (record-path
determinism under burst load — the known replay flake) and the
**M2b-final L1→8 gate**, currently executing in the background
(leg-by-leg markers at `logs/m2b_final_gate.log`).
