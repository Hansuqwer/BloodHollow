# T-151 — Siege + pledge wire & HUD (P0, audit 2/20) — takes epoch 27→28

## Context
Audit findings **B10.6 / B11.2 / F4.1 / P0-2**. `grep -rniE "siege|pledge"
client/src` returns **zero** real hits, and `shared/protocol/messages.md` ends at
`PartyMember = 116`: the siege (holder, window, gate HP, heartstone progress,
crown channel, bands) and the pledge (roster, ranks, emblem, vault) exist only as
server state and chat text. A human at Friday-Night leg 3 cannot see the battle
they are fighting, and `gm siege` — the only readout — is a GM chat verb.

## Scope
- **Wire (S2C)**: `SiegeState` (holderId/holderName, phase: idle/window/battle,
  windowEndTick, battleEndTick, gate HP ×2 + names, heartstone progress +
  attuned flag, crown channel ownerId + deadline, bandCount, taxPct) and
  `PledgeInfo` (id, name, emblem, vault, own rank, roster: name/level/rank/zone).
  Send on login, on zone entry to 6, and on change (delta-cheap; do not spam).
  Bump `PROTOCOL_VERSION` by adding messages (note `kProtocolVersion =
  200 + messageCount`, `tools/protogen/protogen.py:82`).
- **Server**: pack from existing `World` siege/pledge state; no new law, no new
  RNG. Keep `gm siege` / `/pledge who` / `/pledge vault` working.
- **Client**: one siege panel (phase, countdown, gates, heartstone bar, crown
  owner, holder + tax) and one pledge panel (roster, ranks, emblem glyph, vault);
  emblem drawn on member nameplates; crown channel shown as a progress line over
  the kneeling player.
- **Docs**: append the new messages to `messages.md` (F4.1 drift) and note which
  systems remain chat-only.
- **Epoch**: if any journaled command semantics change, bump `kJournalEpoch`
  27→28 and ship a fresh gate leg. Pure client-side rendering of existing state
  should need no bump — prove it by replaying `logs/t146.bwj` (`mm=0`) and say so
  in the PR either way.

## Acceptance criteria
1. `ctest` green; new pins: `SiegeState`/`PledgeInfo` codec round-trip
   (`test_bytestream`/`test_protogen` pattern) + a client-law pin that the panels
   exist and are fed by the wire struct.
2. Live: `--siege-rehearsal` + 8 attackers / 4 defenders → a connected client
   shows gate HP falling on `/breach`, heartstone progress, attune line, crown
   channel, holder flip. Screenshot per stage in the PR.
3. Live: `/pledge create` + promote + tithe → panel shows roster/ranks/emblem/
   vault without typing `/pledge who`.
4. Replay: `logs/t146.bwj` (or the fresh epoch-28 leg) `mismatches=0`.
5. `messages.md` and the client agree with the generated serializers.

## Tests required
Codec round-trip; client-law pins; a siege rehearsal leg script (extend
`tools/t137_m4_leg.sh` or add `tools/t151_siege_hud_leg.sh`) producing the
screenshots; replay `mm=0`.

## Out of scope
Siege law changes (gate HP, crown duration → T-158), EK surface (T-163),
emblem *art* (placeholder glyphs 0–9 are fine).

## Evidence owed at merge
Suite count, leg path + replay line, screenshots (day + night), devlog, board row,
and an explicit statement of whether the epoch moved and why.
