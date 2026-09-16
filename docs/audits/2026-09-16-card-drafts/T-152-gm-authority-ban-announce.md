# T-152 — GM authority: operator allowlist, `/ban`, `gm announce` (P0, audit 3/20)

## Context
Audit finding **F6.1 / B13.2 / P0-3**. There is no GM concept in the codebase
(`grep -rniE "isGm|gmName|operator|admin|BH_GM" server/src` → 0 hits). The verbs
`gm blood-moon`, `gm siege-start`, `gm ek`, `gm siege` are ordinary chat strings
handled with no permission check (`server/src/main.cpp:520,526,635,638`), so
**any logged-in player** can raise a Blood Moon until dawn (night bite 115 % →
130 %, curses ×2 — `world.cpp:1523-1550`) or open the siege battle inside the
window. `docs/ops/gm-runbook.md` also records two missing boxes: **no `/ban`**
and **no free-text announce**, and it cites `gm siege-now`, which the merge
repair deleted.

## Scope
- **Operator identity**: an allowlist the client cannot influence —
  `BH_GM_NAMES` env (comma-separated, case-insensitive) **and/or** a
  `gm_accounts` table (schema v15 if chosen; additive, `user_version`-guarded).
  Refuse every `gm *` line for non-operators with a directed system line and an
  audit print (`[gm-denied] name=… verb=…`).
- **`/ban <name> <minutes>` + `/kick <name>`** (operator-only): session drop +
  login refusal for the duration, persisted so a restart keeps it; one audit line
  per action (`logs/gm.log`, same discipline as `logs/trades.log`).
- **`gm announce <text>`**: ch-2 world line, **journaled** (it is world-visible
  history) or explicitly unjournaled with a reason — pick one and document it.
- **Audit**: extend `docs/ops/gm-runbook.md` (§2 kick/ban, §5 announce, and a new
  "who is a GM" section replacing the current "there is no GM flag" text). Remove
  the dead `gm siege-now` reference.
- OUT: role tiers, web admin, IP bans (the runbook's firewall procedure stays),
  any change to siege/blood-moon law (T-158).

## Acceptance criteria
1. A non-operator client typing `gm blood-moon` gets a refusal; `[moon]` never
   prints. An operator client still raises it. Unit pin for both paths.
2. `/ban` on a connected bot drops it and a reconnect within the window is
   refused (reason code); after expiry it succeeds. Unit + live pin.
3. `gm announce hello` reaches every online session (two-client or bot log
   evidence) and leaves an audit line.
4. Restart with an active ban → still banned (persistence proof).
5. If a schema/wire/epoch change was needed, the PR states it and ships the leg;
   otherwise `logs/t146.bwj` replays `mm=0`.

## Tests required
doctest: allowlist accept/deny, ban expiry, announce routing; live leg with 2
bot waves (one operator-named, one not).

## Evidence owed at merge
Suite count, live log excerpts (denied + granted + ban + announce), runbook diff,
devlog, board row.
