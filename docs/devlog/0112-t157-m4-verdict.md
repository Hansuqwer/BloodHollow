# 0112 — T-157 M4 verdict at epoch 29: FAIL (drill), cause found, fixes carded

2026-09-16. `m4e29` (12v6, 750 s, headless): battles 0, gates 0, attuned 0,
flips 0 → M4 FAIL. Perf PASS (p99 3.0 ms), determinism PASS (15042 ticks,
2155 cmds, 150 hashes, mm=0; leg force-added as replay anchor). Root cause:
`[gm-denied] tick=1494` — T-152's allowlist denied bot0's single-shot
`gm siege-start` (no `BH_GM_NAMES` in drill); `breach()` correctly refused
while inactive. Pre-T-152 drills passed on the ungated verb — drill-vs-tree
interaction, not a sim bug. Filed T-157-F1 (horn: BH_GM_NAMES + retry +
abort-fast, blocker) and T-157-F2 (7 fragmented bands, quality). F1 re-run
owes the re-verdict; 20×1800 s soak + M2/M3 still scheduled. Cards
`done/T-157.md`, `T-157-F1/F2`; readiness doc refreshed.
