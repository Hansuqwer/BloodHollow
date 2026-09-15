# Devlog 0090 — T-129 Blood Moon flag + two levers (H4 night war, 1/3)

## What

Session-scoped red-moon flag to next dawn (`gm blood-moon` → journaled
`kBloodMoon`, H1 shape re-implemented — H1 lives in PR #29's lineage).
One moon at a time + dead callers fail quiet; fiction on chatCh 2;
`[moon]` printf; never persisted.

Levers while red: curse lasts 60 s (`curseDuration()` at slam + bolt
sites) and the night bite lands ×1.30 (`nightBiteNum()` at the T-061
site). Scheduler hook + client red tint are filed follow-ups (Phase S /
art); GM gating waits on accounts (H1 posture).

## Epoch

NONE (stays 24): flag defaults off, no new draws on the moon path — old
journals never contain kBloodMoon. Proven: `t128.bwj` (epoch 24)
re-replays `ticks=1241 cmds=487 hashes=12 mismatches=0` on the new binary.

## Evidence

- `test_blood_moon.cpp`: 6 cases — raise-to-dawn (08:00 → 252000 ticks),
  fiction, quiet repeat/dead, tick expiry, lever values both states,
  journaled path, paired-worlds bite ratio (ghoul, ≈130/115).
- Full ctest 2/2. validate_links 0/5. Duel pin unchanged.
- Test note: Marsh Rat (1001) is aggro-0 passive — paired-combat pins need
  the Feral Ghoul (1002, aggro 6).

## Next

T-130: EK ledger + L19 town oath (persist schema + l-line grammar + GM
`ek` readout). Then Phase S siege battle logic.
