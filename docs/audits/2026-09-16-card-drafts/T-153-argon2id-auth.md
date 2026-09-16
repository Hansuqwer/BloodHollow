# T-153 — Real password hashing (argon2id) + registration posture (P0, audit 4/20)

## Context
Audit finding **B12.3 / F6.2 / P0-4**. `server/src/persist.h` ships
`stubPasswordHash` — a per-account salted iterative **FNV-1a** — with its own
comment: *"NOT cryptographically secure; replaced by argon2id before any public
alpha wave"* (ADR-0009). Registration is **OPEN by default** and the alpha
checklist (MVP §7) requires wrong-version refusal (shipped) plus a boot-from-
clean-machine story (T-149). Invite wave 1 cannot go out on a stub hash.

## Scope
- **ADR first** (AGENTS.md: no new third-party dependency without one):
  `ADR-0012-argon2id-password-hashing.md` — choice, memory/time/parallelism
  parameters, vendoring + pinned hash, alternative considered (bcrypt/scrypt,
  and "keep the stub for a friends-only wave").
- Vendored argon2 (single-directory reference implementation) built directly like
  ENet/sqlite in `third_party/CMakeLists.txt`; no system package dependency.
- `Db`: widen the credential columns additively (schema v15: `kdf` + `kdf_params`
  or a self-describing PHC string), verify with constant-time compare, and
  **rehash on successful login** so existing stub accounts migrate silently.
- Refuse login for accounts whose stored hash cannot be parsed (reason code),
  never crash.
- Registration posture: keep `--no-register` and document the wave-1 default in
  the runbook; the limiter (T-109) stays as-is.
- OUT: password reset email, 2FA, account merge, the launcher (T-146).

## Acceptance criteria
1. ADR merged/approved by the director before code lands (or in the same PR with
   an explicit "approved verbally" note).
2. New account → stored hash is argon2id (PHC string), login works, wrong password
   is refused, 10 consecutive fails trip the 60 s lockout (existing pin).
3. **Migration**: an account created before this card (FNV-1a row) logs in once
   and its stored hash is rewritten to argon2id; a second login uses the new path.
   Unit pin with a fixture DB.
4. Hashing cost measured: p99 login latency under 30 bots logging in inside the
   limiter window (paste the number); server tick p99 unaffected (login is off the
   sim thread or budgeted — say which).
5. `logs/t146.bwj` still replays `mm=0` (auth is outside the sim).

## Tests required
doctest: hash/verify round-trip, wrong password, malformed hash, migration
rewrite, constant-time compare presence; live 30-bot login wave log.

## Evidence owed at merge
ADR file, suite count, migration proof, latency numbers, devlog, board row.
