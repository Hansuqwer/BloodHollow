# 0073 — Critical review hotfixes (T-111)

Agent-driven full-tree review against the post-hardening tree (T-104..T-110
already in code; epoch 18 + `logs/t107.bwj`). Prompt:
[`docs/prompts/critical-code-review-2026-09-12.md`](../prompts/critical-code-review-2026-09-12.md).

## Judgment calls

- **Spatial is law, not cosmetics.** A body at the gallows that the AoI grid
  still parks at the bindstone is a silent PvE/PvP bug (guards, heals, party
  share radius, mob acquire). Fix matches walker and grid; no epoch — hash
  never mixed spatial cells.
- **Anvil pointers die on erase.** Same class as T-106's deque rule, on a
  `vector<InvSlot>`. Index-through-erase (tryRefine already did this for
  junk) is the house pattern; applied to the weapon slot.
- **`stoul` after digit-check is not throw-free.** T-104 closed chat
  `/refine`; login blob is the same remote-DoS family on a different door.
  Checked multiply + length cap; corrupt records skip, never abort the
  process.
- **Spawner `--n` on `uint32_t`.** Replaced with placed/tries counters. Boot
  must not depend on "map is mostly walkable" luck.

## What landed

| Fix | File | Note |
|---|---|---|
| F1 respawn spatial | `server/src/world.cpp` `respawnTick` | `insert(id, home_.x, home_.y)` |
| F2 anvil slot index | `server/src/world.cpp` `tryAnvil` | index adjusted on part erase |
| F3 inv-blob ints | `server/src/world.cpp` `parseInvBlob` | no `stoul` |
| F4 mob seed bound | `server/src/world.cpp` `initialMobSpawns` | maxTries cap |
| F5 brace tidy | `server/src/world.cpp` `killPlayer` | wanted-mark indent |
| tests | `tests/test_t111_review.cpp` | 4 cases |

## Non-findings (already closed)

T-104 parseRefineArg · T-105 trade unequipped · T-106 kill-line snapshots ·
T-107 worldHash economy · T-108 headless client-law gate · T-109 login
limiter · T-110 docs truth-up. Task cards for those still missing under
`docs/tasks/done/` — docs debt, not code debt.

## Deferred

- AoS `deque` → slot+gen store (architecture aspiration).
- Property fuzz for combat/enhance (AGENTS.md aspiration).
- Human trade-pass slice (T-033 / T-099 packet).
