# 0083 — The second collision, and the epoch turns (T-121)

Same lesson, new teeth. The first counter collision (T-117) was docs and a
bot experiment; this one moved the epoch. PR #19 was the weapon-skill
persistence card — schema v11, journal l-line v3, epoch 20→21 — authored as
T-118 from `c8d8087` while master took T-118 for the M3 gate and T-119 for
the wave evidence. Two sessions, one number, again. The collision rule held
without modification: master keeps its numbers, the incoming card renumbers
(T-118→T-120), and everything mapped to the number moves with it.

The new wrinkle was the **binary leg collision**: both sides carried a
`logs/t118.bwj` — master's M3 crypt leg (epoch 20) and their fresh
weapon-skill leg (epoch 21). Bytes are history; you don't re-record a
journal. Resolution: master's file stays byte-identical, theirs renames to
`t120.bwj`, and the replay re-verified under the new name —
`ticks=1500 sessionCmds=529 hashes=15 mismatches=0`. The epoch guard then
did exactly what it exists to do: `t115.bwj` and `t118.bwj` now refuse
(journal 20 vs build 21). Old legs are history, not garbage.

Because this wave touched sim semantics, the battery wasn't optional:
209/209 (329,052 — three new persistence tests), ctest 2/2, duel pin
untouched (`b273be661b54673a` — persistence changed no combat math), fresh
leg clean, guard refusals by contract. CI green on the sync head, both
matrices. Squash-merged as `7b2e9b8`.

The quiet milestone: **the Sisyphus ladder is fixed.** Since T-096 we knew
sword skill reset to zero on every relog — the Soma spine, the thing the
whole aura grind hangs from, was fake progression. As of epoch 21, skill
persists, the journal carries it, and replay proves it. Track B1 is done;
the queue says pledge-lite next (T-122), and the registrar NPC has been
standing in Thornwall with art already shipped, waiting for a card.

Both standing prompts got dated amendments rather than silent rewrites —
epoch 21, schema v11, suite 209/209, leg of record `t120.bwj`, l-line v3
grammar. The board remains the truth; the prompts now agree with it again.
