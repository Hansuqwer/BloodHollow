# ADR-0012 — Argon2id password hashing (replaces FNV-1a stub)

**Status**: accepted (T-153, 2026-09-16 — director-approved wave-2 follow-up)

**Context**: `stubPasswordHash` (salted iterative FNV-1a, ADR-0009) was always
marked "replaced by argon2id before any public alpha wave". Invite wave 1
cannot go out on a stub hash. Alternatives: bcrypt (no memory hardness),
scrypt (weaker GPU story + fiddlier params), keep-stub-for-friends-only
(rejected — the stub must die before strangers arrive, and the wave is the wave).

**Decision**: argon2id, reference implementation 20190702 (P-H-C), pinned
tarball SHA256 `daf972a8...` (full hash in `third_party/CMakeLists.txt`),
built directly like ENet/SQLite (`bh_argon2`: argon2.c, core.c, ref.c,
encoding.c, thread.c, blake2b.c). Params: **m=19456 KiB (19 MiB), t=2, p=1**
(OWASP low-end: ~tens of ms/verify on the alpha box; PHC string stored).
License CC0/Apache-2.0 dual (see `third_party/` note for the T-148 manifest).

**Migration**: `characters`… no — `accounts` gains `pwhash_phc TEXT` (schema
v16). Non-empty PHC → argon2id verify (constant-time in lib); empty → stub
verify with constant-time u64 compare, then **rehash-on-login** (one UPDATE).
Unparseable stored hash → refused (reason 1 path), never crash. T-109 limiter
(30 new/60 s, 10-fail lockout) stays as-is.

**Threading note (honest)**: the server is single-threaded (tick + net one
loop) — each verify burns ~one hash (~tens of ms, inside the 250 ms tick
resync budget). The limiter spreads bursts; logins are rare next to ticks.
Off-thread auth pool is deferred (carded if p99 evidence demands it).

**Consequences**: (+) wave-1 password posture real; (+) silent migration, no
wipe; (−) login bursts cost ticks (bounded, measured in card).
