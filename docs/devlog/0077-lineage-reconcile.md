# 0077 — Two lineages, one epoch ladder (T-115)

The handover's prediction held: PR #9 (T-104 wave) and PR #8+#11 (T-111 +
T-112) both claimed journal epoch 19 from different trees, and
`merge-tree` showed them colliding in `server/src/world.cpp` and
`tests/CMakeLists.txt`. T-115 is the pre-resolved merge, so the director's
button-pressing needs no code judgment.

## What the conflicts actually were

- **Two duplicate bugs.** Both lineages independently found and fixed the
  respawn-at-`spawnPoint` AoI ghost (T-104 #5 ≈ T-111 F1 — byte-identical
  one-liners, different comments) and the `stoul`-throwing inventory blob
  parse (T-104 #6 ≈ T-111 F3 — two different throw-free parses with the
  same contract). Dedup kept one of each; both parents' regression tests
  run together in the merged suite and pass under the survivors.
- **One real interleave.** T-104 #1 (anvil destroy must ERASE the slot,
  not zero it) and T-111 F2 (track the weapon slot by INDEX, never a
  pointer, across the toll loop's part-stack erases) patch the same
  function from opposite sides. The union is cleaner than either parent:
  `e.inv.erase(e.inv.begin() + wslotIdx)` — no pointer scan, no tombstone,
  index honest by construction.
- **The epoch ladder.** 19 was claimed twice; the reconciled tree takes
  20 and refuses both v19 gate legs by contract. `logs/t115.bwj` (20 bots
  × 10 s, p99 1.27 ms, replay `mismatches=0`) is the leg of record; the
  parents' t104/t112 legs stay as history, exactly like t107 did.

## Verification

206/206 cases (194 base + 5 + 4 + 3), 329,022 assertions, ctest 2/2, duel
pin byte-identical (neither lineage touched combat math). The merged tree
also inherits the vendored sqlite, so it builds offline in the sandbox.

## Judgment calls

- Kept T-104's `num()` over T-111's: same observable contract (both
  reject over-long digit fields, both accept in-domain values), and
  keeping the base's version makes this PR's diff read as pure
  T-111-lineage addition.
- Did NOT renumber #11's t112.bwj or rewrite history: the merge commit
  carries the reconciliation; both parents' legs remain replayable against
  their own parent builds (refused by the epoch-20 tree, per contract).
- Merge order is deliberately not chosen here — the PR description gives
  the director a mechanical playbook (merge #9 → this PR; close #8/#11 as
  superseded; #10 rides #9 independently).
