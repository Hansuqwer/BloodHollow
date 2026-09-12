# 0084 — The oath office (T-122)

The registrar was promised a post since T-094 staked out wireKind 72 and
left it unplaced: "Marrowgate and the Weeping Castle do not exist yet."
Neither does Marrowgate today, but pledges do — so the clerk took the
second tile off the town-square spawn point, one past Marta, and the
oath office opened for business.

The design question that shaped everything: **what is a pledge, to the
sim?** Answer: nothing. It's social state. Membership changes no combat
math, no economy, no movement. So pledge state is deliberately outside
worldHash — which means the journal only needs to reproduce *behavior*,
not the registry itself. The c-line grammar carries ints only, so the
pledge NAME cannot ride it; live creation passes the name through
`Command::text` and replay synthesizes `pledge-<id>`. Names are
DB-of-record. The moment names affect hashed state (holdings, taxes —
Phase 4), that changes.

Membership itself needed a persistence lane. Entities are session-
transient; pledges are forever — so the registry keys on character
**name**, the entity carries a two-field cache (`pledgeId`, `pledgeRank`)
restored at login from the schema-v12 columns, and the journal gets a
**g-sidecar** login line (`g tick idx pledgeId rank`, the k-line pattern)
so replay restores the same cache and synthesizes registry stubs for
pledges founded in prior sessions. Old journals have no g-lines and
parse unchanged — that part of the no-bump hope was true.

The hope died anyway, honestly: **the registrar itself is a spawn.** A
new permanent furniture entity in Thornwall shifts every post-boot world
hash, and t120.bwj — the leg of record since this morning — replayed
15/15 mismatches the moment the clerk reported for duty. That is the
T-112 law working as written: world composition changed, so the epoch
turns (21→22) and a fresh leg of record is committed. Old legs refuse by
guard with exit 4 and stay in the repo as history. The battery: 218/218
· 329,270 assertions (nine new cases pinning founding gates, rank law,
chat routing, replay parity, and the v12 migration), ctest 2/2, duel pin
untouched, and the new leg — five bots founding `t122clan`, swearing,
promoting, kicking, leaving, relogging — replaying
`ticks=1222 sessionCmds=105 hashes=12 mismatches=0`.

Two leg lessons worth keeping. First, bots that log in at their **saved**
position don't stand where you think: wave-2 oath-takers materialized at
their wave-1 wander endpoints, outside the walls, and the registrar was
six tiles and a death away. The ceremony now marches to the square
explicitly. Second, pacifists at the town square get eaten by L2 rats —
level means little when you never swing back — so oath-takers swat
whatever gnaws them mid-rite. Era-appropriate.

One GDD deviation, flagged loudly: §8 gates founding on CHA ≥ 20, and no
CHA exists in the shipped stat model. The stand-in is level ≥ 10 +
10,000g (against the 100k full-price toll). Switch the gate when the
six-stat model lands — don't grow a stat for a guild office.

Next: the roster chrome card (wire bump, T-041→T-048 precedent), or the
queue's T-123 — Weeping Castle. The steward's post (73) is still
reserved, still unplaced, still waiting for its castle.
