# T-166 — Bounty board persistence (+ cut pickup-N) (P1, audit 17/20)

## Context
Audit finding **B4.3 / C-cut C5**. The Wanted Board ships as a session-scoped
quarry cycle: *"T-065 session-scoped bounty board (no persistence by design)"*
(`server/src/world.h:438-440`), kill-N only. For a player, a board that forgets
every mark on restart reads as a bug, not a design; and GDD §12's "quests = bounty
board only: kill-N/pickup-N" leaves pickup-N unshipped. Audit §7 C5 recommends
**cutting pickup-N and adding persistence**.

## Scope
- Persist the active mark per character (or per board) across restarts: additive
  schema bump (`bounty_mob_id`, `bounty_cycle`, `bounty_progress`) or one blob
  column; load on boot, save on the canonical logout path (T-049x law).
- Keep the cycle/quarry law and the payout-over-natural-band rule; make the
  board's own text state the remaining count and whether it survives a restart.
- Client: a bounty line in the board panel or a directed readout (reuse the
  existing furniture interaction; no new wire message if a directed system line
  suffices — say which).
- Director decision recorded: pickup-N cut (amend GDD §12) or carded separately.
- OUT: quest chains, NPC dialogue trees, repeatable daily boards.

## Acceptance criteria
1. Take a mark → kill 1 → restart the server → the mark and progress are intact
   (sqlite before/after + live log).
2. Complete it after the restart → payout lands once (no double-pay pin).
3. Unit pins: persistence round-trip, cycle expiry, payout band, refusal when no
   mark is held.
4. Replay `mm=0`; epoch bump only if the bounty enters `worldHash` (decide and
   state — T-122 kept pledge state out by design; follow that precedent
   consciously).
5. GDD §12 amended for the pickup-N cut (T-158 cross-ref).

## Tests required
doctest persistence + payout pins; a live restart probe pasted in the PR.

## Evidence owed at merge
Suite count, restart evidence, replay line, devlog, board row.
