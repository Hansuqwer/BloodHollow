# 0012 — The path-clear that replay never saw

Sprint 11, 2026-09-05. One card: T-049, the record-side replay flake. Closed.

## The hunt

Cadence-25 hash checks had been failing intermittently under 6-bot load:
exactly **1 mismatch of 17**, first at tick ~125, always transient, always the
same magnitude. Fresh evidence this sprint, in order of falsification:

1. **Journal fair-share** — verified clean: zero same-(tick, session) command
   pairs. Not the cause on paper.
2. **Not aura work** — zero-anvil control runs failed identically; an S9
   journal replayed fine; replaying one journal twice is bit-stable.
3. **Dumps lied by omission** — at the first mismatch tick, live and replay
   entity sets, order, tile positions, hp were *identical*, while
   recomputing the FNV world hash from both dumps gave one value and the
   journal stored another. The hash mixes `walker.x/y` in Q10 sub-tile units;
   the dumps printed integer tiles. The divergence lived in a lane we weren't
   printing.
4. **Q10 dumps + every-tick replay dumps + combat-lane fields**
   (`atk/psz/ch/sw/mv/tgt`) pinned the first divergence to tick 111, entity
   292: `path.size()` 0 live vs 1 replay, positions equal. Live dropped the
   walker's last queued tile mid-step; replay popped it naturally 3 ticks
   later and walked 256 units past.

## Root cause

Live `processCommand()` for `kAttack` clears `e.path` **before** calling
`setAttack()` — attack intent cancels manual pathing even when the target is
already dead. The replay switch called only `setAttack()`, whose dead-target
early-return skipped the clear. Bot 0 attacked mob 6 one tick after mob 6
died: live cancelled its walk order, replay kept walking. One 256-unit slip,
hashes diverge, fight geometry re-converges, flake.

Three more latent asymmetries found in the same audit: `kSkill` lost the
`a==0 → attackTarget` fallback, `kUseItem` read `q.a` instead of `channel`
(always 0 from the decoder), `kBuy`/`kTradeItem` lost the `qty>0` guard.

## The fix that can't drift again

`server/src/command.h` — one `applyWorldCommand()`, called by both the live
network path and `--replay-world`. The journal already stores exact a/b/
channel fields, so replay rebuilds the original `Command` and runs the same
code. Two implementation halves that must stay identical are now one file.

## Numbers

- 56/56 tests (new `test_cmdparity.cpp`: dead-target path-cancel, burst vs
  one-per-tick convergence, skill fallback, useItem channel, qty-0 buy).
- Acceptance soaks: cadence-25 × 3 consecutive 300s runs — 0/241 mismatches
  each; cadence-100 floor 0/61; cadence-1 fresh 0/515.
- Both archived failing journals now replay **OK** on the fixed binary.

M2b (L1→8 cross-zone pacing run with wipe replay) is unblocked. Sprint 12:
T-034 (gear-aware duel harness for the balancer) feeding straight into M2b.
