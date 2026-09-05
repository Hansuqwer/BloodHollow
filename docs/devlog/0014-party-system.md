# 0014 — Two can keep a secret if both are farming ghouls

Sprint 13, 2026-09-05. Cards: T-050 (party core), T-051 (party XP share),
T-052 (client party frame). Phase 3 spine opens with grouping.

## Why now

Sprint 12's campaign legs forced the design hand: solo pacing plateaus at L6
(~30 min/level post-L5), and the Soma-era answer was never "grind harder" —
it was *bring someone*. Party XP is the M2b-final lever and the first real
Phase-3 social system.

## The wire (v237)

Party rides the inventory pattern: `PartyReset{partyId,leaderId,count}` → N ×
`PartyMember{id,name,level,hp,hpMax,zoneId}`. `partyId == 0` clears the frame.
C2S is four verbs; invites carry a name, kicks carry an id. The interesting
bit is *where* ids get pinned: the handler resolves `/invite <name>` to an
entityId **before** journaling, so the replay runner never re-resolves names —
journals store ids, replay replays ids, party state is bit-exact. (Or it
would be, if it weren't for… see "the sinkhole" below.)

## Rules, locked

Session-scoped (parties die at logout — no schema, no persistence, friend
lists are M3). Invite: players only, both alive, same zone, ≤12 tiles, leader
only, one pending invite per invitee (200 ticks or renew), cap 8 because M3
raids want a ceiling. Leave/kick unify at one choke; leader walking off hands
the crown to the eldest member; an empty party disbands. Despawn sweeps
membership silently and in join order — replay sees the same roster without a
single extra journal line.

## T-051: the reason to group

Share window is 12 tiles Chebyshev **around the kill** (not around the
killer — spears and levers tax positioning). Payout per sharer:

```
each = xp * (100 + 12 * (n-1)) / 100 / n
```

+12% total pool per extra mouth, split evenly, karma modifier still per-head.
Loot and gold stay with the killer — era rule: party power is XP, gear stays
yours to hock at the anvil. Sharers hear "party share: N xp." on the system
channel so the rate uplift is *felt*, not assumed.

Test pin: two fresh hunters at a Marsh Rat (xp 40) take 22 each
(`40·112/200`), the mate who died mid-hunt gets nothing, the bystander 20
tiles out gets nothing, and a solo kill pays the same 40 it always did.

## The sinkhole we patched standing in it

While wiring slash-verb floats we found a latent horror: `World::tick()`
clears `events_` at entry, and commands are applied *before* the tick — so
any WorldEvent a command emitted (party messages, anvil ceremony floaters…)
was shredded before `distributeEvents` could read it. Unit tests never caught
it because they read `events()` before ticking. `tickServer` now drains the
event buffer right after the command phase and again post-tick. Push-path
only; no hash drift. This is the second time the record/replay discipline has
paid for itself this month.

## Client frame

HB-chrome roster panel, left margin: `*name L<level>` rows, red hp bars,
leader star, own row in era gold. Invisible when unaffiliated. Refreshes once
a second off the stats tick; roster pushes happen instantly on any
`partyChanged`. The whole panel is 40 lines of raylib and zero new state
ownership.

## Gate

2 campaign bots, auto-partied at t0 (even index `/invite`s the sibling, odd
`/accept`s), target L5, 600 s soak, journal on
(`tools/s13_party_leg.sh`, logs `s13_party_leg*`).

**Replay: bit-exact.** 9 879 ticks, 981 session commands, 98 hash markers,
**0 mismatches** — party commands ride the journal like everything else.

**Share fires.** The dings pair off (L2 22/28 s, L3 179/226 s, L4 228/326 s,
L5 556/583 s): one bot swings, *both* advance — kill position within 12 tiles
and alive, by the numbers pinned in the tests.

**Honest pace note.** L5 landed ~556-583 s partied vs ~537 s solo in the S12
chain — the +12% pool buys exactly nothing at the rat/bat bands where one kill
splits two ways; contention and death tax (36 deaths here) dominate. The math
only starts to pay where S12 plateaued: L5+, when the pair can legally walk
into leash-16 pack camps (hounds, gnolls) a solo campaigner has to step
around. That is precisely the M2b-final rerun's question, and it's next.

## Next in queue

Class kits (Cultist first), alignment/PK rules, anvil gear-churn
(durability/refine/affixes), day/night gameplay, Bonehowl Mine + Drowned
Crypt + Gravemother + elites + bounty board, VFX/audio pass 1.
