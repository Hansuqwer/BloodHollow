# 0085 — The siege stage (T-123)

The queue said it plainly: "content shift ⇒ epoch bump." The interesting
part of this card was never the map — it was the **branching question**.

For the first time, the pipeline's next card had an *open predecessor*:
PR #22 (T-122) is still awaiting the director, master still at `818d661`,
and the journal epoch is a globally-unique semantics key. Branch from
master and bump to 22? Then two living builds both call themselves epoch
22 with different worlds — journals that lie. Skip to 23? A phantom gap
if #22 dies unmerged. The honest answer was to **stack**: branch on
#22's head, epoch 22→23, and say so in the PR title. The chain stays
linear (registrar-era 21 → pledge 22 → castle 23), the PR diff collapses
to exactly the castle work the moment #22 merges, and if the director
ever merges this one first, the squash carries T-122 with it — stated,
not hidden. Merge order is a note, not a hope.

The stage itself: 56x44 of bleak moor, a moat with two causeways, a
curtain wall with two one-tile gaps, and the four fixed pieces standing
where the mapgen says they stand — West and East Gates at 100,000 HP
(the one number GDD §8 prices), the Heartstone at the crossroads
(150,000 — a stand-in; the GDD is silent, B4 will tune it), the Weeping
Throne in the keep nook. The garrison is new mob 1015, the Oathbroken
Sentinel — Sexton base plus two levels, the dead watch that never
learned the siege ended. Four spawner camps and a hound pack on the
approach moor. The fields map grew an east-rim opening by the gnoll
camp: the war road runs Bleak Fields → moor → gates.

B4 inherits two deliberate loose ends, both flagged in the card: gates
don't block movement (pathing is map-data law; the gap tiles are
walkable so the breach mechanics can own the question), and nothing
prices a Heartstone yet. The stage is a stage — the law comes next.

The leg told the era's story on its own: five L25 fighters seeded into
the courtyard, and the garrison did not care about levels. Ten kills,
forty swings, two deaths — the walk of shame sends the dead home to
Thornwall, era law, reported as run. Four of five held the castle
through the relog; all five wave-2 logins are zone-6 l-lines; the replay
came back `ticks=1022 cmds=277 hashes=10 mismatches=0`, and the pledge
leg (`t122.bwj`) refuses on this build exactly as the guard demands.
Suite 223/223 (five new castle pins, breach-path walkability included),
ctest 2/2, duel pin untouched. The CI extension (castle determinism
diff + link validator on every push) is written and sitting in the PR
description: this session's credential cannot push workflow edits — a
small toll for the director's key ring.

Next: B4, the siege law — the biggest card in the phase, and the prompt
already says to split it. The gates have their HP; someone has to decide
what a siege ram is.
