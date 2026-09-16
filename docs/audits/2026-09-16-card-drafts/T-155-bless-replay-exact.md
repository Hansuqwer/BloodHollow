# T-155 — `--bless` is replay-exact, or refused while recording (P1, audit 6/20)

## Context
Audit finding **F6.4 / F6.3 / P1-2**, isolated by an A/B control pair on HEAD
`2bd471d` (same waves, same seeds, same ports shape):

| run | replay |
|---|---|
| `--bless isoA__00=3001:16` + 2×4 wander bots | `[replay] FAIL ticks=1901 sessionCmds=57 hashes=20 mismatches=10` |
| identical, **no** `--bless` | `[replay] OK ticks=1901 sessionCmds=57 hashes=20 mismatches=0` |

Live applies the injection while handling `Hello` and journals it stamped
`s.tick + 1` (`server/src/main.cpp:463-467`); replay applies it from
`queuedBlesses` inside the login-application block (`main.cpp:1461`) — a different
phase/tick, so the world stream diverges. No committed leg script uses `--bless`
(`grep -l bless tools/*.sh` → 0 hits), so shipped gate evidence is unaffected, but
AGENTS.md's DoD ("record → replay → `mismatches=0`") silently fails for any blessed
session, and `--bless` is the natural way to stage a gate leg.

Secondary (same card, cheap): `s.bless` is a `map<name, spec>`, so repeated
`--bless name=…` flags **overwrite** each other (observed: 7 flags → only the last
applied); the flag is absent from the usage string, the GM runbook and any audit
log; and its parse uses `std::stoul` (`main.cpp:443,450,454`) which throws on
malformed operator input.

## Scope
- Make the injection replay-exact: apply it through the **same** path in live and
  replay (a journaled command at a defined tick, or fold it into the `l`-line login
  state so there is nothing to re-apply). Pick one and document the choice.
- If exactness is not worth it: **refuse to start** with `--bless` + `--record-world`
  together and print why (fail loudly, never diverge quietly).
- Accumulate repeated `--bless name=…` flags instead of overwriting; add the flag
  to the usage string; write one `[bless-audit]` line per injection (name, spec,
  tick) so an operator grant is traceable.
- Throw-free parse of the spec (the `parseRefineArg`/T-104 pattern).
- OUT: turning `--bless` into a runtime GM verb (that is T-152's `/give`, if ever).

## Acceptance criteria
1. Reproduction: the A/B pair above now gives `mismatches=0` **both** ways (or the
   blessed run refuses to start with a clear message).
2. `--bless a=x:1 --bless a=y:2` grants both (unit or live pin).
3. Malformed spec (`--bless a=99999999999999999999:1`, `--bless a=gold:`,
   `--bless a=nocolon`) → warning, no crash, no throw.
4. `[bless-audit]` line present for each injection; usage string lists `--bless`.
5. `logs/t146.bwj` still replays `mm=0`; no epoch bump unless sim semantics moved
   (state which).

## Tests required
doctest for spec parse + accumulation; a recorded/replayed leg with a blessed bot.

## Evidence owed at merge
Both replay outputs, suite count, devlog, board row.
