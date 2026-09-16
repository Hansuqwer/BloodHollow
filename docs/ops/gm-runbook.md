# GM runbook (alpha) — BloodHollow live ops

Every command below is verified against the tree (chat-verb switch in
`server/src/main.cpp`, World methods in `server/src/world.cpp`). Anything
not listed here does not exist — do not invent slash commands on a live
server. Audit weekly: `logs/trades.log` (dupe trail, T-080) + `logs/gm.log`
(T-152 GM audit) + `logs/bans.log` (T-152 bans) + the unit journal
(`journalctl -u bh-server`).

## 1. Who is a GM

GM verbs are **gated** (T-152). Only accounts on the operator allowlist may
execute them; everyone else receives a directed `gm denied: operator only`
line and a `[gm-denied]` audit entry.

- **Allowlist sources (union, case-insensitive):**
  - Env `BH_GM_NAMES` — comma-separated, trimmed. Example:
    `BH_GM_NAMES=alice,bob,carol` (`alice` == `Alice`). Restart to reload.
  - Table `gm_accounts(name)` — `INSERT INTO gm_accounts(name) VALUES('alice')`
    (COLLATE NOCASE). Loaded at boot; no restart needed beyond `sqlite3` edit
    + `systemctl reload` (or restart). `SELECT name FROM gm_accounts;` lists.
- Empty allowlist = no GM (every `gm *` is denied). Set at least one for ops.
- All `gm *` verbs must be typed by the operator's **logged-in character**;
  keep that character logged out unless operating. Audit: `logs/gm.log`.

**Gated GM verbs (all require GM):**
- `gm blood-moon` — raise Blood Moon until next 05:00 (journaled c-line, replay-exact; hash-neutral like siege)
- `gm siege-start` — open the siege battle (in-window + ≥1 band, else quiet; journaled)
- `gm ek` — direct EK board readout (directed ch 255, unjournaled)
- `gm siege` — direct castle readout holder/vault/crowns/bands (directed, unjournaled)
- `gm announce <text>` — world-wide ch 2 broadcast `[ANNOUNCE] <text>` to every in-world session; **journaled** as `y` line (world-visible history) but hash-neutral (like other ch2 broadcasts). 160-char sanitized.

Non-GM typing any of the above gets `gm denied: operator only (verb)` and
`[gm-denied] tick=… name=… verb=…` in `logs/gm.log` and stdout.

## 2. Kick / ban

- **Party kick (any player, party leader only):** `/kick <name>` — removes from party only. Stays as before; GM's `/kick` is session-level (below) and takes precedence when the sender is a GM.
- **Pledge kick (Liege):** `/pledge kick <name>`.
- **Operator kick (GM only):** `/kick <name>` — session drop (if online). Sends `KickNotice` + disconnect, `logs/gm.log` + `logs/bans.log` line `[kick] tick=… by=… target=…`, world ch2 `was kicked by`.
- **Operator ban (GM only):** `/ban <name> <minutes> [reason]` — persists in `bans` table (`name` PK COLLATE NOCASE, `expires` unix secs, `reason`, `banned_by`), survives restart, `pruneExpiredBans` at boot. Also kicks if online. Audit: `[ban] tick=… by=… target=… minutes=… reason='…' expires=…` to `logs/gm.log` + `logs/bans.log`, world ch2 `was banned for …m`. Login with an active ban is refused `LoginResult(ok=0, reason=7)` + `KickNotice(banned…)` and `[ban-denied]` log. Expiry is wall-clock (`time(2)`); `expires==0` would be permanent (not used by `/ban` which requires ≥1 min, max 5256000 ≈10y).
- **Operator unban (GM only):** `/unban <name>` — deletes `bans` row, `logs/gm.log` + `logs/bans.log` `[unban]`, world ch2 `was unbanned by`.
- **Throttle shield (T-109):** 30 new accounts/60 s/IP + 60 logins/60 s/IP + 10 bad-password fails → 60 s lockout. Leave it on for every public window; mass-login waves must stagger (bots do, players must be told).

Legacy live procedure without `/ban` (pre-T-152: `DELETE FROM accounts` + firewall-drop) is **obsolete** — use `/ban`.

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

- `gm siege-start` (GM, in-window + ≥1 band, else quiet) opens the battle;
  `gm siege` (GM) reads holder/vault/crowns/bands; `/siege-reg` enlists the
  speaker's band (pledge-sworn = whole war-host, T-139); `/breach` rams;
  `/crown` kneels 10 s at the attuned stone.
- Rehearsal override: `--siege-rehearsal` flag (journal epoch-rehearsal).
  `gm siege-now` was removed in the 2026-09-16 merge repair — use the flag.
  Never rehearse on the alpha DB — rehearsals write journals + holder
  state. Drill on a scratch `--db /tmp/...`.
- Markers to grep, not stdout: `breached by`, `attuned at tick`, `crowned:`,
  `band enlisted` (server stdout); broadcasts go to clients.

## 5. Announce / crash reporting

- **Announce (GM):** `gm announce <text>` — world ch 2 `[ANNOUNCE]`, journaled as `y` line (replay-ignored, hash-neutral). Do **not** use plain ch-0 say for ops (AoI only). All other ch-2 world lines are event-driven (`gm blood-moon`, `gm siege-start`, first blood, war-horn).
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
repo (director-owned, MVP §7 box open). **New reason 7** = banned (T-152).

## 7. Registration posture (wave-1 default)

- Passwords are argon2id (T-153, ADR-0012, m=19MiB/t=2/p=1, ~45 ms/verify);
  stub-era rows rehash silently on next login. T-109 limiter still applies
  (30 new accounts/60 s/IP, 10-fail 60 s lockout).
- **Wave-1 default: `--no-register`.** Pre-create invite accounts, then run
  closed. Open registration is for load/soak windows only (announced,
  monitored, limiter briefed). Reason 6 = registration disabled.
