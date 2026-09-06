# 0023 — the relog launderer (T-049x): one inventory grammar for live + replay

## What was laundering

Every M2b-final leg starts from a fresh `bh_server` process reusing the same
campaign DB, so each leg's *first* login is a **relog** from a persisted
7-field inventory blob:

```
iid:qty:equipped:aura:durability:affix:refine;...
```

Two independent parsers consumed those blobs and neither survived contact:

- **Live login** (`main.cpp`, S10-era): `find(':')`/`rfind(':')` heuristic.
  On a 7-field record the "tail" after the *last* colon is just the **refine**
  digit, so `equipped` became `refine == 1`, and aura/durability/affix
  silently defaulted. Worn gear logged back in **unworn**; a refine-1 piece
  logged in **spontaneously equipped**.
- **Replay applyLogin** (`--replay-world`): `sscanf("%u:%u:%u:%u")` +
  `debugGive` lane. The 5th/6th/7th fields (durability/affix/refine) were read
  as nothing at all — so a **dormant (0-durability) weapon logged back in at
  durability 100**, full stats, no cost. That is the launderer: the wipe
  economy's "kept, not destroyed" row resurrects on every relog under replay.
  Worse, `debugGive` re-stacks into the first matching unequipped slot, so
  slot **order scrambled** and equipped flags detoured through first-match
  patching.

Neither side agreed with the other; the divergence just sat *under* the world
hash — `worldHash` mixes only id/zone/pos/hp per entity, so pure inventory
differences stay invisible until they surface through combat output (hp).
The M2b persist-round journals (`t49_run1p/2p`, ≤ 909 ticks) passed replay for
exactly that reason: short window, coarse hash, no gear-flavoured combat
crossing a hash edge in time. T-049's acceptance was green, and it was
accidentally still wrong.

## The fix (single grammar)

- `parseInvBlob()` + `canonicalInvBlob()` in `server/src/world.{h,cpp}` —
  ONE sequential 7-field split, legacy short tails defaulted, slots appended
  in blob order, nothing stacks/reorders.
- Live login and replay `applyLogin` both call it — the same code path, the
  same defaults, the same byte-for-byte slot list. Replay bless grants still
  run *after* the blob, mirroring live login exactly.
- Logout save serializes through `canonicalInvBlob` (field-identical to the
  old inline writer; covered by the round-trip pin).

## The harness bite: a half-written hash line

First full repro pass under the fix: run1/run2 green, run3 persist-round
replay FAILed with **one phantom mismatch at the final hash** — expected
`000000000a52a7e3`, got a full-entropy hash. The expected value's zero high
bytes were the tell: the journal's last line was literally
`h 875 a52a7e3` — **seven** hex digits. The persist-round server is
SIGTERM-killed the moment its bots finish, and the block-buffered journal
lost its unflushed tail mid-line. The replay then dutifully verified against
garbage.

Two-part remedy, both in the record/replay contract:

- `setvbuf(s.journal, nullptr, _IOLBF, 0)` at journal open — complete lines
  survive an ungraceful kill; content unchanged, so the epoch stays 5.
- The replay now refuses a hash field that isn't exactly 16 hex digits:
  `[replay] truncated hash line at tick 875: 'h 875 a52a7e3' — journal tail
  cut mid-write?` and exits 4, instead of failing later with a phantom
  mismatch on otherwise-consistent data.

## Probes (BH_DUMP_ENTS=1)

- `[live-rng]`, `[live-ply]`, `[replay-rng]`, `[replay-ply]` fingerprint the
  RNG stream and per-player progression+inventory at the hash cadence (same
  post-tick phase both sides — grep one tick to compare).
- `[live-login]` / `[replay-login]` print the post-application login state
  incl. the canonical inv, so the launderer is caught the instant a relog
  disagrees.
- `tools/bh_probe_leg.sh` runs a cadence-1 smoke leg and **diffs the whole
  probe stream** live-vs-replay (verdict
  `logs/probe_leg_verdict.txt`).

## Tests

`tests/test_inv_blob.cpp` (7 new cases, suite 98→105): full 7-field
round-trip in order, dormant-gear survival, no-stacking/reordering under
duplicate ids, legacy short-tail defaults, malformed-skip, canonical
round-trip losslessness, and a combat-lane pin (an equipped refine-2 blade
parsed from a blob drives `debugWeaponDmg` exactly like a live grant).

## Gates (2026-09-06, epoch 5 build)

- Suite **105/105** (327,925 assertions); ctest 2/2.
- Every committed journal replays 0 mismatches under the fixed binary:
  `t49_run1/2` (+persist rounds), `m2b_final_leg3..12` (ticks 8192–10795).
- `tools/t49_repro.sh` fresh `T49_VERDICT fail=0` — see `logs/t49_verdict.log`.
- `tools/bh_probe_leg.sh` `PROBE_VERDICT equal` — live and replay
  fingerprints match line-for-line.
- Journal epoch stays 5: the login grammar change only re-interprets the
  persisted blob fields, and every pre-existing journal still replays
  byte-identical (verified above).

## Standing

Persistence and record/replay now share one grammar, under one test file, with
a probe leg that diffs the two lanes directly. Any future
`saveProgress`/`applyLogin` edit keeps the ink: if it touches the blob, the
inv-blob pins and the probe leg are both expected to catch it before commit.