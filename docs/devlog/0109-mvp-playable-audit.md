# 0109 — MVP playable audit at epoch 27 (docs only, no code)

Ran `docs/prompts/mvp-playable-audit-2026-09-16.md` end to end against HEAD
`2bd471d` (epoch 27, schema v14, protocol 237). **Verdict: NOT-PLAYABLE** for the
Friday-Night test. 6 P0 / 15 P1 / 13 P2 / 9 P3 / 11 UNVERIFIED.

Artifacts: `docs/audits/2026-09-16-mvp-playable-audit.md` (phase results A–H, P0
cut line, sequencing, defer/cut list, risk delta),
`docs/audits/2026-09-16-mvp-requirement-ledger.md` (flat per-requirement table
with probe → observed → verdict → severity → owner → effort → card), and 20 card
drafts T-150..T-169 in `docs/audits/2026-09-16-card-drafts/` (T-146..T-149 were
already filed on `task/human-only-mvp-cards`, so numbering starts at 150).

The six P0s: (1) client has no `mapFileFor` case 6, so Weeping Castle draws as
Thornwall; (2) siege + pledge have no wire and zero client awareness — chat verbs
only; (3) `gm blood-moon` / `gm siege-start` / `gm ek` / `gm siege` are ungated
chat strings — no GM concept exists anywhere, and there is no `/ban` or announce;
(4) passwords are still the FNV-1a stub ADR-0009 promised to replace before a
public wave; (5) a stranger cannot discover the 18 chat verbs or most hotkeys;
(6) the world is coloured diamonds — T-ART-12..15 were ruled at B0 and never filed.
Gate-blocker: the only replayable leg of record is `logs/t146.bwj` (5 wanderers,
10 s); M4/M1/M2/M3 evidence all predates the merge repair, and the ops runbook
still pins epoch 26 and cites `gm siege-now`, which no longer exists.

Live probes that passed (scratch DBs, quoted in the report): siege rehearsal
8v4/210 s → both gates breached, replay `mm=0`, p99 733 µs; pledge ceremony
T-138-shaped → ranks/vault persist across relog, schema v14, integrity ok; SIGKILL
mid-run → journal tail intact, clean restart; 403 garbage datagrams → server
alive, 0 errors; 30- and 40-bot soaks → p99 0.17 / 0.30 ms against a 10 ms budget.

New defect found while probing: `--bless` breaks replay determinism. A/B control
on identical waves — with the flag `mismatches=10/20`, without it `0/20`. Live
applies the injection during `Hello` and journals it at `s.tick+1`; replay applies
it at the login-command phase. No committed leg uses `--bless`, so shipped gate
evidence is clean, but it is a trap for anyone staging a leg (T-155).

Two audit-environment limits, recorded rather than papered over: the sandbox has no
X11/GL dev headers and no working apt mirror, so the client and `bh_tests` could
not be built or run locally — A2/A3 evidence is CI run 35059645385 (green, both
OSes) and static reading; and `cmake --preset headless` cannot build `bh_tests`
because `tests/test_zoom.cpp` pulls `raylib.h` and `tests/CMakeLists.txt` links
raylib unconditionally, so headless agents get zero unit coverage (T-154).

Nothing merged, nothing closed: the PR queue is triaged (merge #49→#50→#51,
cherry-pick `tests/test_castle.cpp` + `tools/t123_castle_leg.sh` out of #23 before
closing it, close #22/#30 as superseded, director call on #27/#28) but left for the
director — T-165.
