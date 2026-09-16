# GM runbook (alpha) — BloodHollow live ops

Every command below is verified against the tree (chat-verb switch in
`server/src/main.cpp`, World methods in `server/src/world.cpp`). Anything
not listed here does not exist — do not invent slash commands on a live
server. Audit weekly: `logs/trades.log` (dupe trail, T-080) + the unit
journal (`journalctl -u bh-server`).

## 1. Who is a GM

There is no GM flag on accounts (prototype posture). GM verbs are chat
lines typed by the operator's own logged-in character on the live server.
Keep the operator character logged out unless operating; all GM chat is
visible to players in range (say) or world (broadcasts).

## 2. Kick / ban

- **Party kick (any player):** `/kick <name>` — removes from party only.
- **Pledge kick (Liege):** `/pledge kick <name>`.
- **No `/ban` verb exists** (verified 2026-09-15 — follow-up carded).
  Live procedure: `--no-register` restart to stop fresh accounts, then at
  the host: `sqlite3 world.bhdb "DELETE FROM accounts WHERE name='<x>'"` +
  firewall-drop the IP + note it in the audit log. Unban = remove the rule.
- **Throttle shield (T-109):** 30 new accounts/60 s/IP + 60 logins/60 s/IP
  + 10 bad-password fails → 60 s lockout. Leave it on for every public
  window; mass-login waves must stagger (bots do, players must be told).

## 3. Rollback

- Point-in-time = last `bh_backup.sh` snapshot + journals since. Procedure:
  1. `systemctl stop bh-server`
  2. `bash tools/ops/bh_backup.sh restore /var/backups/bloodhollow/world-<STAMP>.bhdb /var/lib/bloodhollow/world.bhdb`
  3. Copy that stamp's journals aside (they stay valid history).
  4. `systemctl start bh-server`, verify boot line + `gm siege` readout.
- Rollforward alternative (surgical): `--replay-world <journal>` replays a
  known-good journal to confirm the sim, not to restore accounts (replay
  never touches the Db). Rollback is restore-from-backup, always.
- **Drill, not just written:** `bash tools/ops/bh_backup.sh drill`
  (evidence: T-143 — integrity ok + table counts on the restored copy).

## 4. Siege (weekly 90-min, Saturday 20:00–21:30 game time)

- `gm siege-start` (in-window + ≥1 band, else quiet) opens the battle;
  `gm siege` reads holder/vault/crowns/bands; `/siege-reg` enlists the
  speaker's band (pledge-sworn = whole war-host, T-139); `/breach` rams;
  `/crown` kneels 10 s at the attuned stone.
- Rehearsal override: `gm siege-now` (H1 stub) / `--siege-rehearsal` flag.
  Never rehearse on the alpha DB — rehearsals write journals + holder
  state. Drill on a scratch `--db /tmp/...`.
- Markers to grep, not stdout: `breached by`, `attuned at tick`, `crowned:`,
  `band enlisted` (server stdout); broadcasts go to clients.

## 5. Announce / crash reporting

- **Announce:** plain channel-0 say reaches AoI only. There is NO free-text
  GM broadcast verb (verified 2026-09-15 — follow-up carded). The only
  world-wide (ch-2) lines the server emits are event-driven (`gm
  blood-moon`, `gm siege-start`, first blood, war-horn) — all carry sim
  side effects, so none is usable as a pure announcement. Workaround:
  say it in Thornwall plaza at peak, or card a `gm announce` verb.
- **Crash reporting:** unit captures stdout/stderr to the journal
  (`StandardOutput/StandardError=journal`); cores via
  `coredumpctl -u bh-server`. Weekly: `coredumpctl --since "1 week ago"`
  must be empty; any core → pin the build hash + journal tail + file a
  card before the next window. No in-process crash handler exists by
  design (a segv handler that mallocs/logs re-crashes — journald IS the
  handler).

## 6. Wrong-version clients

Refused at handshake by construction: `Hello.protoVersion` mismatch →
`LoginResult(ok=0, reason=4)` (messages.md). Launcher duty is to fetch the
manifest first so players never see reason=4. There is NO launcher in the
repo (director-owned, MVP §7 box open).
