# 0063 — Sixty-three anvils (T-094)

T-094 placed three NPCs and fixed sixty-three anvils. The second half is the story.

## How a passing test caught a live-world bug

The placement sweep failed 0-vs-2 on a fresh arena. The code read correct — so I traced, and the trace said `ents=68`: sixty-three Widow Anvils where one belongs. `spawnAnvils` carries the exact failure the T-069 comment describes for Marta (bare `break` exits only the dx loop; every ring row seeds another anvil), but nobody ever fixed the anvil variant because no test counted anvils and every live entity total quietly absorbed ~100 of them. The T-083 "live-entity growth 330→378" watch was measuring anvil spam alongside aggro coverage — the growth was partly a bug all along.

## What landed

- `spawnNpcs()`: twins + two post guards, spiral scans, art-flagged derived positions (anvil-flank, gate posts).
- One-anvil-per-zone fix (Marta pattern). Entity counts should drop ~100 live; p99 should improve; the T-083 watch numbers rebase (noted, not rewritten — history stays).
- Epoch 13→14 (both changes are entity-count content changes).

## Judgment calls

- Fixed the anvil bug inside this card instead of opening a new one: same diff region, same epoch, and leaving 63 anvils standing while placing twins "flanking the anvil" would have been absurd (which of the 63?). Stated here so the change is reviewable as its own line.
- Twins as two entities (not one shared cell): the brief allows either; two adjacent tiles reads as flanking with zero art input, and the shared-anchor variant remains a one-line art-side call later.
- Guards as furniture, not patrols: they stand their posts like every other NPC. Arming them is a behavior card with its own epoch.
