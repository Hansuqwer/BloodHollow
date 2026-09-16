# MVP-playable audit — 2026-09-16

**HEAD** `2bd471d3cf3f0aa4b131cacd341e9480c83b5332` (single-commit history) ·
**journal epoch 27** · **schema v14** · **protocol 237** (37 messages) ·
**branch** `arena/01a0a8c9-bloodhollow` · prompt
[`docs/prompts/mvp-playable-audit-2026-09-16.md`](../prompts/mvp-playable-audit-2026-09-16.md) ·
ledger [`2026-09-16-mvp-requirement-ledger.md`](2026-09-16-mvp-requirement-ledger.md) ·
card drafts [`2026-09-16-card-drafts/`](2026-09-16-card-drafts/)

---

## 1. Verdict

**NOT-PLAYABLE** — for the MVP §6 Friday-Night Test with real humans.
The *simulation* is playable and healthy; the *game a human sees* is not.

| | count |
|---|---|
| **P0** (blocks a human playing/certifying the MVP) | **6** |
| **P1** (MVP IN row materially incomplete or illegible) | **15** |
| **P2** (drift, evidence, ops friction) | **13** |
| **P3** (hygiene) | **9** |
| **UNVERIFIED** (needs a display / X11 / macOS / 12 h wall) | **11** |

**Build capability this session:** headless-only. `cmake`/`ninja` installed via
pip (`--break-system-packages`); **no X11/GL dev headers and the apt archive is
unreachable**, so raylib could not be fetched → `bh_client` and `bh_tests` were
never built or run locally. Server, bots, mapconv and duel harnesses built
warning-free and were exercised live. Graphical/behavioural rows are marked
`UNVERIFIED (no display)` with a `RUN-FIRST` battery in §11.

**Three worst findings, in plain English**

1. **A player cannot see the siege.** The client has zero siege/pledge code
   (`grep -rniE "siege|pledge" client/src` → no real hits), no wire messages for
   it (the protocol stops at `PartyMember = 116`), and `mapFileFor()` has no
   `case 6` — so a human who walks into Weeping Castle is drawn **Thornwall**
   with no gates, no heartstone, no holder, no countdown, no crown progress. The
   server-side siege works (proved live: bands enlisted, both gates breached,
   replay `mm=0`), but Friday-Night leg 3 ("ownership flips and it is
   screenshot-worthy") cannot pass.
2. **Anyone can be a GM.** There is no GM concept in the code (`grep -rniE
   "isGm|operator|admin|BH_GM" server/src` → 0 hits). `gm blood-moon` and
   `gm siege-start` are ordinary chat strings handled with no permission check,
   so any logged-in player can raise a Blood Moon until dawn (night bite 115 % →
   130 %, curses ×2) or open the siege battle. There is also no `/ban` and no
   announce verb, and passwords are still the ADR-0009 FNV-1a stub.
3. **The world is coloured diamonds.** 709 terrain PNGs, all mob/NPC sheets and
   every palette sit in `assets/aigen/` while the client draws flat
   `terrainColor()` diamonds, untextured prisms, the hero placeholder for every
   player (only 2 of 18 player sheets are packed), **0 VFX and 0 icons**, and the
   default raylib font. The four engine cards that would make any of it visible
   (**T-ART-12/13/14/15**) were ruled at the B0 gate and never filed. Design
   pillar 1 ("era-authentic feel") is unmet regardless of how complete the sim is.

**Estimated agent-sessions to clear P0: 9–13**, of which **2 need a wire change**
(T-151 siege/pledge wire; T-150's client-side is wire-free) and **2 need a
schema/dep ADR** (T-152 GM whitelist, T-153 argon2id). One epoch bump (27→28) is
unavoidable and should be taken **once**, early, by the wire card.

---

## 2. What is already true (do not re-audit)

Verified live on this tree at epoch 27 — these are solid:

- **Determinism spine.** Fresh record→replay `ticks=901 cmds=17 hashes=10
  mismatches=0`; leg of record `logs/t146.bwj` `ticks=242 hashes=2 mm=0`;
  four historical legs (epoch 26/22/18) all refuse with **exit 4** as designed.
- **Siege sim + rehearsal determinism.** 8 attackers + 4 defenders, L15 top-up,
  210 s: 4 bands enlisted, `battle joined (6 bands)`, **Outer Gate + Inner Gate
  breached**, bot telemetry `reg=8 breach=240 crown=8`, tick **p99 733 µs**
  (budget 25 ms), rehearsal journal `ticks=4300 cmds=635 hashes=43 mm=0`.
- **Pledge-lite end-to-end.** 5 bots, 3 waves: `pledges = (1, 't122clan',
  emblem 1, liege audg__00, vault_gold 500)`; after relog the rank matrix is
  exactly T-138's expectation (liege 3 / bloodsworn 2 / initiate 1 / kicked+left
  0); journal `v 27` with 15 `g`-sidecar lines and kinds 34–41; replay
  `ticks=1202 hashes=12 mm=0`; `PRAGMA integrity_check = ok`, `user_version = 14`.
- **Crash posture.** `SIGKILL` mid-run → journal tail intact (line-buffered),
  restart on the crashed DB with 0 error lines, pre-crash journal replays
  `ticks=1616 hashes=16 mm=0`.
- **Perf headroom.** 30 fighter bots × 60 s: `FINISH ticks=4001 p50=73 p99=172 µs`
  → `soak OK p99=0.17 ms budget=10.00 ms`.
- **Garbage-UDP resilience.** 403 malformed datagrams (0–1400 B random + ENet-ish
  headers with absurd lengths) → server alive, 0 error lines. (Weak signal: no
  protocol fuzz harness exists — §8 F1.3.)
- **Maps & links.** 6/6 `.bhmap` built and validated by the `bh_maps` target;
  `validate_links.py`: 0 problems across 6 maps; **all 6 generators reproduce
  their committed `.tmj` byte-identically**.
- **CI green on both OSes for HEAD** (run 35059645385: ubuntu-latest 3 m 44 s,
  macos-latest 50 s).
- Content that *is* shipped server-side and tested: combat law + crits/slams/
  telegraphs, 14 mobs + 3 nameds + boss, parties + XP share, 9 kit channels,
  anvil/aura/refine/durability, vendors + fence, trades with commit-time
  validation + audit log, karma/duels/guards/wanted/chapel, day/night + light
  items, 6 zones + portals, bounty board, town oath + EK, blood moon, siege
  scheduler/gates/heartstone/crown/taxes, pledge create/emblem/ranks/chat/bands/
  vault, ore nodes + `/mine`.

---

## 3. Phase results

### A — Build / boot / clean machine

| # | Check | Observed | Verdict | Sev |
|---|---|---|---|---|
| A1 | Headless build | `bh_server`, `bh_bots`, `bh_mapconv`, `bh_duel` build clean. **`bh_tests` fails**: `tests/test_zoom.cpp` → `engine/render/camera_rig.h:3: fatal error: raylib.h: No such file or directory`. `tests/CMakeLists.txt` has **no `BH_BUILD_CLIENT` guard** and does `target_link_libraries(bh_tests PRIVATE raylib)` unconditionally | **PARTIAL — headless test gate is broken** (contradicts AGENTS.md DoD + T-108 + the `headless` testPreset) | **P1** |
| A2 | Linux graphical build | UNVERIFIED locally: no X11/GL headers, apt archive unreachable, `tools/bootstrap.sh` fails as written (`apt-get download libwayland-dev wayland-protocols pkg-config` → *Unable to locate package*; `pip install cmake` needs `--break-system-packages` on this image) | UNVERIFIED (CI proves it builds) | P2 (bootstrap) |
| A3 | macOS build | CI job 104677050859 green in 50 s | SHIPPED (CI-proxied) | — |
| A4 | Test suite | Local: UNVERIFIED (no raylib). Static: **303 `TEST_CASE` macros across 59 files**; `tests/CMakeLists.txt` compiles 58 (incl. `tests_main.cpp`); `tests/test_siege_stub.cpp` on disk, **not compiled**. CI ctest green both OSes | PARTIAL evidence | P2 |
| A5 | Map artifacts | 6/6 `.bhmap` produced by `bh_maps` (walkable 83.9 / 86.7 / 42.9 / 26.0 / 31.1 / 86.6 %) | SHIPPED | — |
| A6 | Mapgen determinism | **6/6 byte-identical** to committed `.tmj`. But 4 generators (`make_fields_overflow`, `make_thornwall_crypt`, `make_bonehowl_mine`, `make_drowned_crypt`) **ignore `--out` and write in place** into `data/maps-src/` → CI's diff pattern is only extendable to 2 of 6, and running a generator mutates the working tree | SHIPPED + tooling gap | P2 |
| A7 | mapconv validate | 6/6 clean via the `bh_maps` target | SHIPPED | — |
| A8 | Portal/link integrity | `validate_links.py`: `0 problems across 6 maps` | SHIPPED | — |
| A9 | Server boot | 6 zones online; `[journal] recording … (epoch 27)`; `[auth] registration OPEN … 60 logins + 30 new accounts per 60s per IP, 10 bad-password fails -> 60s lockout`; `pledges: 0 registered` | SHIPPED | — |
| A10 | Bots connect | 5/5 online, `rc=0`, `[bots] OK` | SHIPPED | — |
| A11 | Record→replay | `ticks=901 sessionCmds=17 hashes=10 mismatches=0 entities=210` | SHIPPED | — |
| A12 | Epoch guard | `t140`(26) `t138`(26) `t139`(26) `t122`(22) `t107`(18) → all **exit 4** with the right message | SHIPPED | — |
| A13 | Leg of record | `logs/t146.bwj` → `mm=0`. **But it is 242 ticks / 2 hashes / 5 wander bots × 10 s** — no siege, pledge, refine, trade, mine, PK or cross-zone coverage exists at epoch 27 | SHIPPED-but-thin | **P1** |
| A16 | Clean-machine path | Three conflicting build-dir laws: `tools/bootstrap.sh` → `build/` (Release), `README.md:95-105` → `build/linux-gcc`, every `tools/*_leg.sh` → hard-coded `build/linux-gcc`. `assets/maps/` does not exist until a build runs `bh_maps`. Overlaps director card **T-149** (clean-machine boot) | PARTIAL | **P1** |
| A18 | CI coverage | Gates only: thornwall mapgen diff, configure, build, ctest, thornwall mapconv validate. **Not gated**: replay legs, soak, headless preset, any client run/screenshot (no xvfb path exists), 5 of 6 mapgens, 5 of 6 mapconv validations, artifact upload | PARTIAL | P2 |

### B — MVP §1 IN rows (full decomposition in the ledger)

| Row | Verdict | Decisive evidence | Sev |
|---|---|---|---|
| B1 World (5 maps + castle) | **PARTIAL** | 6 zones server-side ✓; **client `mapFileFor()` has cases 2–5 + `default: thornwall`, no case 6** (`client/src/game.cpp:971-978`) → castle renders as town | **P0** |
| B2 Classes / kits / cap 25 / 3 pts | **PARTIAL** | `kLevelCap = 25` ✓ (`shared/sim/combat.h:21`); 3 kits ✓; **9 channels vs ~23 GDD skills**; **`assignStat` accepts 0/1/2 only** (STR/VIT/DEX, `world.cpp:338-344`), INT/MAG kit-seeded, **CHA does not exist** (`world.cpp:2994-2995` admits it; `world.h:295 kPledgeMinLevel`); class chosen by `/kit` chat verb — **no creation UI, no sex field anywhere** | **P1** |
| B3 Combat | **SHIPPED (invisible parts)** | server-auth ✓, hit/crit/kill ✓, potions ✓; gore = 0.6 s vestige + decals header; telegraphs are server events with **no client anim state for attack/cast/hurt/die on players** (T-ART-04 hook exists, player sheets don't) | P2 |
| B4 Progression | **PARTIAL** | XP debt + de-level ✓; tiers ✓ but recomputed multipliers drift from ×8/×20/×100; **bounty board is session-scoped, kill-N only, no pickup-N, not persisted** (`world.h:438-440`) | P1 |
| B5 Party | **SHIPPED** | cap 5, invite/accept/leave/kick, XP share + party frame; 8-man absent ✓ (OUT) | — |
| B6 The Anvil | **PARTIAL / DRIFTED** | cap +7 ✓; **`kRefineChance[7] = {100,100,60,65,50,35,25}` vs GDD `{100,90,80,…}`**; **SHATTER at `refine==2` failure (`world.cpp:2162`) is not in GDD §7 at all**; **no ore purity, no fodder tiers** (`grep -rn "purity\|fodder\|510[123]"` → 0 hits); refine eats a monster part + 50 g, not ore; mining ✓ (`spawnOreNodes` zone 4, `/mine`, kind 74); durability + `/repair` ✓; glow tiers 1 (+5..9) / 2 (+10+) exist while the cap is +7 → **tier 2 unreachable** | **P1** |
| B7 Loot | **PARTIAL** | **rarity: 0 occurrences in the tree** vs 4 tiers; **affixes 10** (`kAffixCount`) vs ~40; **gear slots 2** (weapon/armor) vs GDD's 11; uniques **12 rows / 4 bosses ✓**; trade + audit log ✓; inv-blob grammar single-path ✓ | **P1** |
| B8 PK | **PARTIAL** | karma/bands/gallows/duels/guards/chapel ✓; town oath at L19 + EK persisted ✓ but **the only EK readout is the GM-only verb `gm ek`** — no player surface, no page (T-147 filed); Marrowgate exists as an identity + 1 guard kind, **no enemy-town population to fight** → the town war is theoretically live, practically unreachable | **P1** |
| B9 Day/night | **PARTIAL** | 4 h clock ✓, tint ✓ (peak α150), night economy ✓, nightOnly spawners ✓ (`bhmap` v2 + `world.cpp:244,4112`); **no additive light mask** (T-ART-03 phase 2), GDD's own §9 note says light radius shipped **0** → verify what a human sees at 04:00 with a torch (UNVERIFIED, no display) | P1 |
| B10 Siege | **SHIPPED-INVISIBLE** | Scheduler/window math correct (`kSiegeDayTicks=288000` = 4 h real, `kSiegeLenTicks=108000` = 90 real min, `kSiegeStartOff` = Sat 20:00); gates/heartstone/crown/taxes/holder ✓ and live-proven; **gates are 300 HP ram work, not 100k HP with siege-damage skills ×3** (`world.h:368`); **crown channel 10 s, not GDD's 60 s** (`kCrownChannelTicks=200`); attune step (60 s uncontested, `kHeartCaptureTicks=1200`) is not in the GDD; **flip not reproduced in a 210 s drill** (attuned=0, crowned=0 — defenders contest the ring); **zero client surface** | **P0** (client) + P1 (law/evidence) |
| B11 Pledges lite | **SHIPPED-INVISIBLE** | Live-proven at epoch 27 incl. vault tithe + relog restore (§2); **emblem/roster/vault have no client surface**; founding gate L≥10 + 10 000 g is a documented GDD §8 deviation (CHA ≥20 + 100k) with **no ADR**; vault spend deferred (deposit-only) | P1 |
| B12 Account | **PARTIAL** | **1 character per account** (`persist.cpp` `SELECT … WHERE account_id=? LIMIT 1`, one INSERT per account) vs MVP's 4 + offline slots; login/version-refusal ✓ (`reason=4`); rate limits ✓; **password = salted iterative FNV-1a stub** (`persist.h`: "replaced by argon2id before any public alpha wave") | **P0** (auth) + P1 (slots) |
| B13 Ops | **PARTIAL** | backup + drilled restore ✓, systemd unit ✓, journald/coredumpctl posture ✓, soak suite ✓ (8 profiles); **no launcher/patcher** (T-146), **no `/ban`**, **no `gm announce`**, **no GM authority at all**, no crash-upload path for remote players, no metrics/alerting/disk cron | **P0** (GM authority) + P1 |
| B14 OUT-list compliance | **1 creep** | **Blood Moon shipped** though GDD §10 lists it [v0.2] and MVP §1 OUT does not include it — and it shipped *partially* (curse ×2 + night bite 130 %; **no spawn ×2, no Pale Sow, no loot tier**). No vampire/auction/stalls/8-man/Windows/localization/+8..+10 found. Weapon auras + use-based sword skill exist — reconcile against the OUT row "usage-based weapon %-skills" (judgment: GDD §3 mastery spine, keep, but record the reading) | P2 |

### C — GDD numeric delta (highlights; all 18 rows in the ledger)

| Spec | GDD/MVP | Shipped | file:line |
|---|---|---|---|
| Stats | 6 (STR VIT DEX INT MAG **CHA**), 3 pts/level | 3 assignable, INT/MAG kit-seeded, CHA absent | `world.cpp:338`, `kits.h` |
| Skills | ~23 across 3 kits | **9 channels** (ch1–ch9) | `world.h:251-258`, `kits.h` |
| Rarity | 4 tiers 78/17/4.6/0.4 % | **none** | — |
| Affixes | ~40 | **10** | `items.h:70` |
| Gear slots | 11 | **2** | `items.h:13` |
| Uniques | ~12 boss/named | **12** ✓ | `items.h` `kUniqueDrops` |
| Refine odds | 100/90/80/65/50/35/25 | **100/100/60**/65/50/35/25 | `world.cpp:2153` |
| Refine failure | −1 level (+4..+6), reset at +7 | **SHATTER at +2→+3**, −1, reset at +6→0 | `world.cpp:2162-2173` |
| Ore | purity 1–10 + fodder tiers ±% | **absent** (ore item + `/mine` only) | grep 0 hits |
| Gate HP | 100 000 + siege skills ×3 | **300** + `/breach` ram | `world.h:368` |
| Crown channel | 60 s | **10 s** (after a 60 s uncontested attune) | `world.h:373-374` |
| Pledge creation | CHA ≥20 + 100k g | **L≥10 + 10 000 g** | `world.h:295` |
| Mob roster | 12 common + 2 elite + 3 named + 1 boss | 14 rows; missing Pale Cultist, Mine Wretch, Lantern Spider, Mud Golem, Crypt Revenant, Grave Banshee, Bell Ringer; 1007 rename (D9 "Waxen Celebrant") not applied | `mobs.h` |
| Leash | 18 tiles | 10–16 per row | `mobs.h` |
| Blood Moon | spawn ×2, Pale Sow, +loot tier | curse ×2 + night bite 130 % only | `world.cpp:1523-1550` |
| Economy sinks | potions, repair, refine, **teleport scrolls**, upkeep, castle tax | teleport scrolls **absent** | grep 0 hits |
| Art budget §11 | 18 player sheets, ~60 icons, ~25 VFX, 40 SFX | **2** player sheets, **0** icons, **0** VFX PNGs, ~10 synth voices | `find assets/aigen -name '*.png'` |

### D — Playable-loop walkthrough

**Could not be run as a human session** (no display, no client binary). Executed
instead as: live server + bot waves for every step that has a bot profile
(pledge ceremony, siege choreography, fighter economy, wander/relog), plus
code-path reading for the client-facing half. Each of the 24 steps carries
`UNVERIFIED (no display)` or a bot-proven substitute in the ledger; §11 lists the
exact 40-minute script for the director to run on a real machine.

Two walkthrough facts established without a display:

- **Discoverability (P0-5 → T-169).** The client's help chrome is two lines
  (`game.cpp:1586-1588`): `LMB walk/fight - 1 PowerSwing - Q sip - I bag -
  T trade (P commit/X cancel) - Enter chat` / `I inventory - V vendor - Space
  recenter - F3 grid - ESC quit`. The server accepts **18 chat verbs**
  (`/accept /breach /confess /crown /duel /forfeit /invite /kick /kit /leave
  /mine /oath /p /pledge /refine /repair /repent /siege-reg`) plus skills 2–5,
  stat keys F5/F6/F7, anvil F, sell-junk G and buy F1–F12 — **none of which
  appear in any in-client help**. A stranger cannot find the class oath, the
  anvil refine syntax, mining, duels, pledges or the siege without out-of-game
  documentation. → **P0-5**.
- **Debug keys are live for players**: `H`/`N` shift the local hour offset,
  `F3`/`F4` toggle grid/path overlays (`game.cpp:118-121`).

### E — Client legibility

| Area | On disk | On screen | Verdict |
|---|---|---|---|
| Terrain | 709 PNGs (plates, D12 edge sets, prism skins, manifests) | flat `terrainColor()` diamonds + untextured prisms (`game.cpp:728,749`) | **invisible — needs T-ART-12 (never filed)** |
| Mobs | 10 packed sheets (1001–1010) | drawn via `atlasFor(wireKind)` ✓ | SHIPPED for 1001–1010; **1011–1014 (Gate Guard, Old Maw, Red Widow, Cantor Vex) have no sheets** |
| NPCs/furniture | kinds 64–66 shipped; 67–73 `atlas.draft.json` (+ some `sheet.png`/`portrait.png`) | procedural proxies (T-ART-B5) | PARTIAL |
| Players | **2** packed sheets (`ravager/{m,f}`) + 40 raw plates; D7 ruled **18** | **hero placeholder for everyone** (`mobSheetPaths(0)` pinned FALSE) | MISSING — T-142 in PR #49 (mergeable) |
| VFX | **0 PNGs** (spec ~25) | callout text + decals + refine glow | MISSING — needs T-ART-14 (`loadAtlas` rejects `dirs:1`, `atlas.cpp:29`) |
| Icons | **0 PNGs** (spec ~60) | text-only panels | MISSING — needs T-ART-15 |
| Font | `callout_font.png/.fnt` designed, not shipped; `bhfont.py` exists | no `LoadFont`/`DrawTextEx` anywhere → default raylib font | MISSING — needs T-ART-13 |
| Siege/pledge/EK UI | n/a | **nothing** (0 grep hits in `client/src`) | MISSING — needs wire first |
| Audio | 7-voice procedural kit (~78 KB), `BH_NO_AUDIO`-safe, ~10 wired callouts | plays on events ✓ | PARTIAL vs GDD's ~40 SFX + ambience (no footsteps/ambience/music/UI) |
| Window | `InitWindow(1024,768)`, zoom snapped {1,1.5,2} ✓ | no resize/fullscreen/scale path found | P2 (M5 min-spec box) |

### F — Server / persistence / security / determinism

| # | Result | Sev |
|---|---|---|
| F1.1 | Authority coverage table built from `command.h` dispatch: movement, attack range/CD, kit channel unlock + MP, item use, buy/sell price+stock+proximity, trade symmetry, anvil toll, refine slot, mine proximity+tool, party/pledge rank permissions, duel consent all resolve server-side | — |
| F1.2 | `std::stoul` survives in **`server/src/main.cpp:443,450,454`** — the `--bless` operator parse path (not client-reachable), but it throws on malformed operator input at boot | P3 |
| F1.3 | **No fuzz harness** (AGENTS.md truth-up confirmed); garbage-UDP probe survived, protocol-level malformed-packet coverage is untested | P2 |
| F2 | RNG law clean (0 hits for `rand()/random_device/srand/mt19937` in `server/src shared client/src`); wall-clock confined to the shell/limiter/telemetry; `applyWorldCommand` single path; deque-erase snapshots pinned by tests | — |
| F2.5 | Epoch discipline verified live (A11–A13) | — |
| F3 | Schema ladder v1→v14 migrates an empty file cleanly (`user_version=14`, `integrity_check=ok`); full `CharacterRow` round-trips incl. pledge/town/ek; single `parseInvBlob` grammar; **stack merge only when `qty+qty ≤ stackMax`** (`world.cpp:673-675`) → a 10-stack + 10 becomes two stacks rather than 16+4 (bag-space cost, era-plausible: record as behaviour, not a bug) | P3 |
| F4.1 | **Protocol spec drift**: `messages.md` = 37 messages, last id **116**; everything after parties (karma band, glowTier, EK/oath, siege, pledge, mine, blood moon) rides existing messages or chat text. Systems with **no wire representation at all**: siege state, pledge roster/emblem/vault, EK board | **P1** |
| F4.2 | `kProtocolVersion = 200 + len(messages)` (`protogen.py:82`) — implicit versioning; two trees with equal message counts but different layouts collide | P2 |
| F4.3 | `shared/protocol/gen/messages_gen.{h,cpp}` committed but unused (build generates into `${CMAKE_BINARY_DIR}/generated`, `shared/protocol/CMakeLists.txt:5`) | P3 |
| F5 | Per-tick allocation + AoS `deque` debt unchanged (documented); `world.cpp` = **176 KB**, `main.cpp` = 77 KB, `game.cpp` = 67 KB — velocity risk for the P0 work | P2 |
| **F6.1** | **No GM authority model**: `gm blood-moon` / `gm siege-start` / `gm ek` / `gm siege` are unprefixed chat strings with **no permission check** (`main.cpp:520,526,635,638`); `grep -rniE "isGm|gmName|operator|admin|BH_GM" server/src` → 0 hits. Any player can raise a Blood Moon until dawn (`bloodMoon()` guards only "one at a time" and "not dead") or open the siege battle in-window | **P0** |
| F6.2 | Password = `stubPasswordHash` (salted iterative FNV-1a); registration OPEN by default; no `/ban`; no `gm announce`; no crash-upload path | **P0/P1** |
| F6.3 | `--bless` is an unauthenticated-at-runtime operator injection (item/gold/skill) with **no audit line**, **undocumented** in the usage string and the GM runbook, and `s.bless` is a `map<name,spec>` so **repeated `--bless name=…` flags silently overwrite** (observed: 7 flags → only the last applied) | P2 |
| **F6.4** | **`--bless` breaks the replay oracle** (isolated A/B, same waves and seeds): with `--bless isoA__00=3001:16` → `[replay] FAIL ticks=1901 cmds=57 hashes=20 mismatches=10`; without → `[replay] OK … mismatches=0`. Live applies the injection during Hello handling and journals it stamped `s.tick + 1` (`main.cpp:463-467`); replay applies it from `queuedBlesses` inside the login-application block (`main.cpp:1461`) — a different phase/tick. **No committed leg script uses `--bless`** (`grep -l bless tools/*.sh` → 0), so shipped gate evidence is unaffected, but AGENTS.md's DoD silently fails for any blessed session | **P1** |
| F6.5 | `assets/LICENSES.md` exists and covers aigen lanes; completeness per PNG not machine-checked (needs a manifest-vs-disk diff — carded) | P2 |

### G — Gate metrics

| Gate | Target | Fresh evidence at epoch 27? | Verdict |
|---|---|---|---|
| M1 (20-bot soak p99 / desyncs) | <10 ms / 0 | **Yes (partial)**: 30 fighters × 60 s → p99 172 µs, `soak OK budget=10.00ms`; 5 wander × 12 s → p99 128 µs. Not the 30-min shape | PASS-signal, re-run at 30 min |
| M2 (TTK 6–10 s, ~1.2 lvl/h at L6–8) | — | **No**: last evidence is T-113/T-114-era; needs `bh_duel` + a campaign leg at epoch 27 | STALE |
| M3 (party ≥1.4× solo; +5 refine EV ≈45–60 min) | — | **No**: `tools/m3_*` legs predate epoch 27; the refine-EV metric **cannot be computed as specified** (no ore purity / fodder tiers) | STALE + uncomputable |
| M4 (40 bots: p99 <25 ms, flips 3/3) | — | **Partial**: my 12-bot 210 s drill gave p99 733 µs ✓ but **0 attunes / 0 flips**; the 3/3 claim is devlog 0098 at epoch 25 with 750 s and 12v6 | **NOT RE-PROVEN — re-run `tools/t137_m4_leg.sh` at epoch 27 (needs `build/linux-gcc` + ~14 min)** |
| M5 (200 bots × 12 h; 60 fps 2018 MBA) | — | **No**: T-144 banked 60×10 min at epoch 26; fps needs hardware | DIRECTOR-SCHEDULED |
| §6 legs 1–4 | Friday-Night | Legs 1–2 runnable; **leg 3 blocked by P0-1/P0-2**; leg 4 survey. `docs/ops/friday-night-readiness.md` pins "epoch 26, schema v14" and instructs `gm siege-now` — **that verb was dropped by the merge repair** (only `gm siege-start`, `gm siege`, `gm ek`, `gm blood-moon` exist) | **STALE + one dead instruction** |
| §7 alpha checklist | 7 boxes | launcher (T-146) ✗ · systemd+backup drill ✓ · GM runbook ✓-with-gaps (`/ban`, announce) · data-driven spawn tables ✓ (hot-reload ✗) · social/EK page (T-147) ✗ · legal (T-148) ✗ · clean-machine boot (T-149) ✗ | **1.5 / 7** |

### H — Repo / process hygiene

| # | Finding | Verdict |
|---|---|---|
| H1 | **8 open PRs.** `mergeable=UNKNOWN`: #22 (T-122 pledge-lite, epoch 22), #23 (T-123 castle, epoch 23), #27, #28, #30. `MERGEABLE`: **#49 (T-142 wire class/sex, +148/−7, 12 files) ← #50 (T-R-LUMA, base = #49's branch) ← #51 (art probe, base = #50's branch)** — a 3-deep stack that must merge in order | triaged below |
| H2 | **#22 → CLOSE-SUPERSEDED** (pledge core landed via the T-138 cherry-pick; proved live in §2). **#30 → CLOSE-SUPERSEDED** (affix v2 is in master: `kAffixCount = 10`, `test_affix_v2.cpp` compiled). **#27/#28 → docs/art lanes, rebase or close on director call** | — |
| H2b | **#23 must NOT be closed blindly**: it carries **`tests/test_castle.cpp`** and **`tools/t123_castle_leg.sh`**, neither of which exists in master (master's castle coverage is the four `test_siege_*.cpp` files only). Cherry-pick the test + leg before closing | **P1** |
| H3 | History is **1 commit** → no `git blame`, no `bisect`. Future debugging of the 176 KB `world.cpp` will be archaeology-free but evidence-free | P2 |
| H4 | Board truth: `README.md` §Status still says "Next: Phase-4 pledges/siege spine" (shipped); T-138..T-145 cards sit in `docs/tasks/` root with "→ done on merge" rows; T-104/T-112 cards were closed by PR #51 on a branch, not master | P3 |
| H5 | `tests/test_siege_stub.cpp` on disk, not compiled (kept "as evidence") | P3 |
| H6 | `.git` = 101 MB / 2298 tracked files (PNG-heavy); no LFS/asset-split decision on record | P3 |
| H7 | `.gitignore` lists `logs/*.bwj` while 118 legs are tracked → **a new gate leg needs `git add -f`** or it is silently lost | P3 |
| H8 | 27 single-use prompts in `docs/prompts/`, 107 devlogs, no index; **devlog id collisions across parallel branches** (#49 ships `0107-…`, #50 ships `0108-…`, master's latest is 0106 → this audit files **0109**) | P3 |
| H9 | AGENTS.md aspirations still accurately flagged (fuzz absent, SoA deferred); **new drift to add**: the headless test gate (A1) and the `--bless`/replay interaction (F6.4) contradict the DoD as written | P2 |
| H10 | **ADRs owed** for shipped deviations: refine shatter law, pledge gate L10+10k, Blood Moon inside MVP, 1 char/account, CHA-less stat model, argon2id dependency, gate HP 300 + 10 s crown | **P1** |

---

## 4. The P0 cut line (minimum playable set)

Six items (P0-5 = **T-169**, character-creation half = **T-167**). Everything else is P1 or lower. Two need wire work, two need a
schema/dep ADR, and **one epoch bump (27→28) should be taken once** by T-151.

| # | Problem (one line) | Evidence | Fix shape | Effort | Owner | Deps | Wire/schema/epoch |
|---|---|---|---|---|---|---|---|
| **P0-1** | Client renders Thornwall for zone 6 → the siege stage is invisible | `game.cpp:971-978` (no `case 6`) vs `main.cpp:1278,1628` | Add `case 6: weeping_castle.bhmap` + a `test_clientlaw` pin + a headless screenshot | **S** | agent | — | none |
| **P0-2** | Siege + pledge have no wire and no client surface | `grep -rniE "siege\|pledge" client/src` → 0; `messages.md` ends at 116 | New S2C `SiegeState` (holder, window/battle end tick, gate HP ×2, heartstone progress/attuned, crown channel owner+deadline, band count) + `PledgeRoster`/emblem/vault; client HUD panel + nameplate badge; **epoch 27→28 + fresh gate leg**; reuse `gm siege` readout text as the panel's first draft | **L** | agent | P0-1 | **wire + epoch** |
| **P0-3** | Any player can fire GM verbs; no ban, no announce | `main.cpp:520,526,635,638`; 0 hits for a GM concept | Operator allowlist (env `BH_GM_NAMES` or a `gm_accounts` table → **schema v15**), refuse `gm *` for everyone else, add `gm announce <text>` (ch-2, journaled) + `/ban <name> <mins>` with an audit line; update `docs/ops/gm-runbook.md` | **M** | agent | — | **schema (maybe) + epoch if journaled** |
| **P0-4** | Passwords are the FNV-1a stub; registration OPEN by default | `persist.h` (`stubPasswordHash`, "argon2id before any public alpha"); ADR-0009 | **ADR for the argon2id dependency** → hash + salted verify + migration-on-login (rehash on success), keep the limiter, default `--no-register` for public windows; document the wave-1 posture | **M** | agent + director (ADR) | ADR | schema (hash column widen) |
| **P0-5** → **T-169** | A stranger cannot discover the game: 18 chat verbs + most hotkeys invisible, no class/char creation UI | help chrome `game.cpp:1586-1588`; verb list from `main.cpp` | In-client `/help` verb + a persistent verb/key panel (era parchment), first-spawn system lines walking a new character through `/kit`, stats, vendor, anvil, party (**T-169**); character-creation screen **or** an explicit MVP cut to 1 char/account (see §7, **T-167**) | **M** | agent | — | none (help) / wire (creation) |
| **P0-6** | The world is coloured diamonds: terrain, VFX, icons, font and 16 of 18 player sheets never reach the screen | §7 Phase E table; T-ART-12/13/14/15 ruled at B0, never filed | **File and run T-ART-12 (textured ground + prism skins + D12 edge lookup), T-ART-13 (bitmap font), T-ART-14 (`dirs:1` VFX strips), T-ART-15 (icon/hotbar draw)**; then merge PR #49→#50→#51 for class/sex sprites | **L** (4 cards) | agent (engine) + art lane | PR #49 stack | none |

**P0-gate (blocks certifying, not playing):** refresh the epoch-27 evidence —
re-run `tools/t137_m4_leg.sh` (750 s, 12v6) for the M4 flip, one 20-bot × 30-min
M1 soak, one M2/M3 pacing leg, and record a **substantive** epoch-28 leg of
record (siege + pledge + refine + trade + cross-zone, not 5 wanderers × 10 s).
Carded as T-157; without it the Friday-Night go/no-go list cannot be signed.

---

## 5. P1 — the genre-fantasy leaks

| # | Finding | Card |
|---|---|---|
| P1-1 | Headless preset cannot build tests (raylib-linked `test_zoom.cpp`, no `BH_BUILD_CLIENT` guard) → agent sessions without X11 have zero coverage, AGENTS.md DoD unverifiable | T-154 |
| P1-2 | `--bless` diverges record vs replay (10/20 hashes) → determinism oracle silently fails for any blessed session | T-155 |
| P1-3 | Gate evidence at epoch 27 is 242 ticks / 2 hashes; M4 flip, M2/M3 pacing unproven on the merged tree | T-157 |
| P1-4 | Refine law drifted from GDD §7 (odds +2/+3, SHATTER at +2, no purity/fodder) with no ADR; glow tier 2 unreachable at a +7 cap | T-158 |
| P1-5 | Loot depth: no rarity tiers, 10 affixes vs ~40, 2 gear slots vs 11 | T-159 |
| P1-6 | Stat model: 3 assignable stats, no CHA, INT/MAG frozen; pledge gate already re-cut because of it — decide six-stat model or ADR the 5-stat cut | T-160 |
| P1-7 | Kit gap: 9 of ~23 skills; **Resurrect (the GDD's "reason every party wants one")**, Sanctuary, Raise Skeleton, Corpse Explosion, Terror, Mana Shield, Frost Spike, Wither, Curse of Weakness, Sunder, Bull Rush, War Stomp, Executioner, Second Wind | T-161 |
| P1-8 | Mob roster: 7 GDD mobs missing; 1011–1014 have no sheets; D9 rename unapplied | T-162 |
| P1-9 | Town war unreachable in practice (no Marrowgate population) + EK has only a GM-only readout | T-163 |
| P1-10 | Blood Moon: partial vs GDD (no spawn ×2 / Pale Sow / loot tier) and inside MVP scope without change control | T-158 (law) + §7 cut |
| P1-11 | Night light law contradictory (GDD says radius 0 shipped, no additive mask) — what a human sees at 04:00 is unverified | T-164 |
| P1-12 | PR queue: 5 stale PRs, a 3-deep stack, and **#23's `tests/test_castle.cpp` + `t123_castle_leg.sh` never landed** | T-165 |
| P1-13 | Bounty board: session-scoped, kill-N only, no pickup-N, not persisted | T-166 |
| P1-14 | Account: 1 char/account vs MVP's 4 + offline slots; no sex field anywhere (blocks sprite work) | T-167 (or the §7 cut) |
| P1-15 | Clean-machine path: three build-dir laws, `bootstrap.sh` broken as written, `assets/maps/` only exists post-build | T-168 (pairs with director card T-149) |

## 5b. P2 / P3 (condensed; full rows in the ledger)

P2: CI gates 1 of 6 maps and no replay/soak/visual leg · protocol spec drift
(`messages.md` at 116) · `kProtocolVersion = 200 + count` implicit versioning ·
no fuzz harness · `world.cpp` 176 KB single file · SoA/alloc debt · `--bless`
overwrite footgun + undocumented + unaudited · no window scaling/fullscreen ·
no teleport-scroll sink · no spawn-table hot reload · no metrics/alerting/disk
cron · ADR backlog (7 deviations) · asset-licence manifest not machine-checked.
P3: committed-but-unused `shared/protocol/gen/` · uncompiled
`test_siege_stub.cpp` · `.gitignore` vs tracked legs (`git add -f`) · 101 MB
`.git` · README/board staleness · `friday-night-readiness.md` pins epoch 26 and
cites the deleted `gm siege-now` · `std::stoul` in the `--bless` parse ·
stack-merge behaviour · devlog-id collisions across branches · usage string
omits `--record-world`/`--replay-world`/`--bless`.

---

## 6. Sequencing & sprint plan to playable alpha

**Rule: wire/schema/epoch changes first** (they invalidate everybody's journals),
then server-only law, then client-only, then art (parallelisable), then director
boxes. Weeks are from approval, at the director's ~12 h/wk.

| Wave | Weeks | Cards | Gate re-proved | Playable demo at the end |
|---|---|---|---|---|
| **0 — merge hygiene** | 0 (½ wk) | T-165: merge #49→#50→#51 in order; cherry-pick `tests/test_castle.cpp` + `t123_castle_leg.sh` from #23; close #22/#30 as superseded; rule on #27/#28 | CI green, suite count restored | — |
| **1 — the expensive spine** | 1–2 | **T-151** siege/pledge wire + HUD (**takes epoch 27→28 once**) · **T-152** GM authority + `/ban` + `gm announce` (schema v15) · **T-153** argon2id (+ADR) · **T-150** client zone-6 map | new epoch-28 leg of record (siege + pledge + refine + trade + cross-zone) + replay `mm=0` | "Stand at the castle and watch the flip on a HUD" |
| **2 — trust the oracle** | 2–3 | T-154 headless test gate · T-155 `--bless` replay fix (or ban it while recording) · T-157 evidence refresh (M4 750 s, M1 30 min, M2/M3 pacing) · T-158 GDD law truth-up + ADRs | M1/M4 re-proven at epoch 28 | "Siege flips 3/3 with humans watching a HUD" |
| **3 — the loop a stranger can play** | 3–5 | T-167/T-155 creation flow (or the §7 cut) · T-160 stat decision · T-161 support-kit skills (Resurrect first) · T-159 loot depth (rarity + 2 more slots) · T-166 bounty persistence · T-163 town-war reachability + player EK readout | M2/M3 pacing + party ≥1.4× | "Two 5-man parties, one Cultist, race to 12" |
| **4 — the era look** | 3–7 (parallel with 3) | T-ART-12 terrain renderer · T-ART-13 font · T-ART-14 VFX strips · T-ART-15 icons · T-162 mob roster/sheets · T-164 night light law | screenshot set at zoom {1,1.5,2}, day + night | "A screenshot worth posting" |
| **5 — alpha ops** | 7–9 | T-168 clean-machine path · director cards **T-146** launcher · **T-147** site/EK page · **T-148** legal · **T-149** clean boot · M5 200×12 h soak + MBA fps | §7 checklist 7/7 · M5 | **Friday-Night Test** |

Critical path: **T-151 (wire/epoch) → T-152/T-153 (schema) → T-157 (evidence) →
Friday-Night.** Art (wave 4) is parallel and is the largest volume, but it is not
on the critical path for *playable* — it is on the path for *worth inviting
people to*.

## 7. Defer / cut recommendations

MVP §1 change control: *"adding anything to IN requires cutting an IN item of
equal cost + an ADR."* The tree has already added Blood Moon (OUT/[v0.2]) and
weapon auras without that sentence being honoured. Proposals, most valuable first:

| # | Cut / defer | What breaks in the fantasy | What it saves | Recommendation |
|---|---|---|---|---|
| C1 | **4 chars/account → 1** (today's reality) | Alts for the Friday-Night legs (10 humans = 10 chars is enough); no offline char slots | A creation/select screen, schema work, offline-slot sync — ~1 session + wire | **Cut for alpha, ADR it**, restore post-alpha. Pair with capturing **class + sex at creation** (needed by sprites anyway) so the one screen you build is the one that matters |
| C2 | **~40 affixes → 20**, **11 gear slots → 5** (weapon, armor, helm, amulet, ring) | Loot variety halves; paper-doll depth drops | Two content waves + inv-blob/persist churn; keeps rarity tiers affordable | **Cut to 5 slots + 20 affixes, ship 4 rarity tiers on top** (tiers are the visible fantasy; slot count is not) |
| C3 | **Blood Moon → defer to v0.2** (delete the player-triggerable verb, keep the code path behind a GM-only flag) | One event | Removes a P0 grief lever, a scope-creep row, and an unshipped GDD promise (spawn ×2 / Pale Sow / loot tier) | **Defer + ADR**; if kept, it must be GM-gated and either finished to GDD or re-specified |
| C4 | **Refine: adopt the shipped law as the spec** (100/100/60/65/50/35/25 + shatter at +2→+3) *or* revert to GDD §7 | Either the GDD or the tuning story changes | A balance re-run + re-journaling | **Adopt shipped, amend GDD §7, ADR the shatter** — it is playtested and harsher stakes fit pillar 3. Add ore purity (1–10) only if the M3 refine-EV metric is worth keeping; otherwise **cut purity + fodder tiers from the GDD** |
| C5 | **Pickup-N bounties → kill-N only**, persisted | One quest verb | A content + persist lane | **Cut pickup-N, add persistence** (a board that forgets every restart reads as a bug, not a design) |
| C6 | **Launcher/patcher → version manifest + pinned download URL for wave 1** (T-146 stays open, ships for wave 2) | Players patch by hand; reason=4 still refuses wrong versions | The single largest director-owned box | **Defer to wave 2** with a written one-page install/patch instruction |
| C7 | **EK web page → in-game `/ek` readout** (T-147 shrinks to Discord + devlog) | Leaderboard is not public | A site + hosting | **Ship the player-visible readout now** (it is also the fix for P1-9), defer the page |
| C8 | **Six stats → keep 5 (STR VIT DEX INT MAG), ADR the CHA cut**; hard-code aura radius 12 and pledge gate L≥10 + 10k | CHA-driven social build variety | A stat migration + rebalance of every CHA hook in the GDD | **ADR the 5-stat model** — it is already the shipped truth and two systems were re-cut around it |
| C9 | **M5 200×12 h soak → 100×4 h + a scheduled 12 h run the week before the night** | Less soak confidence | 8 h of wall time on the critical path | **Compress once**, keep the full run before invites |

## 8. Risk-register delta (vs MVP §8)

| Risk | Status | Evidence |
|---|---|---|
| R1 netcode perf/desync late | **Green** | p99 0.17–0.73 ms across four live runs; replay `mm=0` on 3 of 4 recordings; epoch guard works |
| R2 art volume explosion | **FIRED** | 709 terrain + 0 VFX + 0 icons + 2/18 player sheets on screen; four engine cards (T-ART-12..15) never filed — the bottleneck moved from *production* to *renderer* |
| R3 scope creep toward "real MMO" | **FIRED (mild)** | Blood Moon shipped partial and out-of-scope; weapon auras + use-based mastery shipped against an OUT row reading; no change-control ADRs |
| R4 part-time burnout | Amber | Velocity is ~5× plan, but 8 stale PRs + a 3-deep stack + 1-commit history is integration debt, not speed |
| R5 party/support math lands wrong | **Amber → unmeasured** | No epoch-27 M3 evidence; the support kit is missing Resurrect/Sanctuary, i.e. the very skills the 1.4× claim depends on |
| R6 cheaters/dupe at alpha | **RED** | No GM authority (any player can fire `gm *`), no `/ban`, stub password hashing, registration OPEN by default, `--bless` unaudited |
| R7 macOS friction | Amber | CI builds green; never run there (no client smoke test on any OS in CI) |
| R8 empty-world problem | Amber | Bot profiles are rich (8), but the castle/siege is invisible to humans, so bot-filled presence won't read as drama |
| **NEW: client-legibility debt** | **Red** | The server is ~2 phases ahead of the client; every "SHIPPED-INVISIBLE" row in the ledger is this risk |
| **NEW: evidence staleness across an epoch bump** | **Amber** | Only `t146.bwj` (242 ticks) replays at 27; M2/M3/M4 claims predate the merge repair |
| **NEW: single-file `world.cpp` (176 KB) + no history** | **Amber** | Every P0 server card lands in the same two files with no `blame`/`bisect` |

## 9. Docs truth-up list (do not edit in this audit — file as one docs card)

1. `README.md` §Status: "Next: Phase-4 pledges/siege spine" → pledges + siege
   shipped (T-131..T-140); the real next is the client-legibility + GM-authority
   wave. Also `logs/t107.bwj` is quoted as a gate leg but now refuses at epoch 27
   (exit 4) — replace with the epoch-28 leg from T-157.
2. `docs/ops/friday-night-readiness.md`: "epoch 26, schema v14" → 27/14 (28 after
   T-151); **`gm siege-now` does not exist** (dropped by the merge repair) →
   `gm siege-start` inside the window, or `--siege-rehearsal`.
3. `docs/ops/gm-runbook.md`: add that **`gm *` verbs are currently unauthenticated**
   (P0-3), document `gm ek`, and note `--bless` exists, what it does, and that it
   must not be used while recording.
4. `shared/protocol/messages.md`: append every post-party message/field actually
   on the wire (karmaBand, glowTier, town/EK, mine, siege, pledge) or state
   explicitly which systems are chat-only; note the `200 + count` version rule.
5. `docs/02-gdd.md`: §7 refine table + shatter + purity/fodder, §8 gate HP
   (100k → 300 ram) and crown channel (60 s → 10 s + 60 s attune), §9 Blood Moon
   as shipped, §3 six-stat model (CHA), §6 leash 18 vs 10–16, §11 roster/art
   counts — each with an ADR reference (T-158).
6. `AGENTS.md`: DoD says "all tests pass" and cites the headless gate — record
   that the headless preset cannot build tests today (T-154), and that
   `--bless` + `--record-world` is not a legal combination (T-155).
7. `docs/tasks/README.md`: move T-138..T-145 cards to `done/`, retire the
   "Next: Phase P" rows, and add the T-150..T-169 queue from this audit.
8. `docs/art/00-VERIFY.md`: rows 5, 6, 8, 10, 19, 22, 24 are now shipped
   (T-ART-01/02/04/05/08/10/11) — re-verify and mark; add rows for T-ART-12..15.

## 10. Card drafts index

All drafts in [`2026-09-16-card-drafts/`](2026-09-16-card-drafts/). Numbering
starts at **T-150** because **T-146..T-149 are already filed** on
`task/human-only-mvp-cards` (launcher/patcher · site+social+EK board · legal-lite ·
clean-machine boot) — this audit deliberately does **not** duplicate them.

| Card | Title | Sev | Effort | Wire/schema/epoch |
|---|---|---|---|---|
| T-150 | Client renders zone 6 (Weeping Castle) | P0 | S | none |
| T-151 | Siege + pledge wire & HUD (epoch 27→28) | P0 | L | **wire + epoch** |
| T-152 | GM authority: operator allowlist, `/ban`, `gm announce` | P0 | M | schema v15 (+epoch if journaled) |
| T-153 | argon2id password hashing + ADR + registration posture | P0 | M | schema + **ADR** |
| T-154 | Headless preset builds and runs the test suite | P1 | S | none |
| T-155 | `--bless` is replay-exact (or refused while recording) | P1 | S | none |
| T-156 | File + run T-ART-12..15 (terrain, font, VFX strips, icons) | P0 | L | none |
| T-157 | Epoch-28 evidence refresh: M4 flip, M1 30-min, M2/M3 pacing, substantive leg of record | P0-gate | M | none |
| T-158 | GDD law truth-up + ADRs for 7 shipped deviations | P1 | M | none |
| T-159 | Loot depth: 4 rarity tiers + 5 slots + 20 affixes | P1 | L | schema (blob) |
| T-160 | Stat model decision: six-stat or ADR'd five-stat | P1 | M | schema |
| T-161 | Support-kit spine: Resurrect, Sanctuary, Purify parity | P1 | L | none |
| T-162 | Mob roster completion + sheets for 1011–1014 + D9 rename | P1 | M | none |
| T-163 | Town war reachable + player-visible EK readout | P1 | M | wire (readout) |
| T-164 | Night light law: decide, implement, screenshot | P1 | M | none |
| T-165 | PR queue: merge the #49→#51 stack, cherry-pick #23's tests, close superseded | P1 | S | none |
| T-166 | Bounty board persistence (+ cut pickup-N) | P1 | S | schema |
| T-167 | Character creation: class + sex at creation (1 char/account, ADR'd) | P1 | M | wire + schema |
| T-168 | One build-dir law + working bootstrap + clean-clone script | P1 | S | none |
| **T-169** | **In-client discoverability: `/help`, verb panel, first five minutes** — drafted while carding P0-5; it is a P0, not a nicety | **P0** | S | none |

## 11. Audit deviations & UNVERIFIED rows

**Deviations**

- No code, wire, schema, epoch, board or GDD edits were made. **No `AUDIT-UNBLOCK`
  commits were needed.**
- Environment changes outside the repo (not committed): `pip install
  --break-system-packages cmake ninja`; `build/headless/` and `assets/maps/*.bhmap`
  are gitignored build output; scratch DBs in `/tmp`; scratch journals/logs in
  `/tmp` (nothing new committed under `logs/`).
- Four map generators write in place, so running them touched `data/maps-src/`;
  all six were confirmed **byte-identical** to HEAD afterwards (`git status`
  clean apart from this audit's new files).
- One probe (the `--bless` economy run) initially looked like a broad determinism
  failure; it was isolated with an A/B control pair before being reported
  (F6.4). Three non-blessed recordings — wander, pledge/sqlite-seeded, siege
  rehearsal — all replay `mm=0`.

**UNVERIFIED (11) + RUN-FIRST battery for the director**

Needs a display/X11, macOS, or long wall time. In order:

```bash
# 1. graphical build + suite (Linux, needs X11/GL dev libs — CI has them)
cmake --preset linux-gcc && cmake --build --preset linux-gcc -j$(nproc)
ctest --preset linux-gcc --output-on-failure        # expect 100%, ~298+ cases

# 2. the headless gate this audit found broken (expect FAIL until T-154)
cmake --preset headless && cmake --build --preset headless   # bh_tests: raylib.h

# 3. client in world + the zone-6 bug (expect: castle draws as Thornwall)
./build/linux-gcc/server/bh_server --db /tmp/rf.db --port 7777 &
./build/linux-gcc/client/bh_client --server 127.0.0.1 --name rfauditor
#    walk the castle portal; screenshot; then grep the help lines vs /help-less verbs

# 4. the 24-step walkthrough (§6 of the prompt) as a human, ~40 min

# 5. M4 re-proof at current epoch (~14 min, scratch DB, never the alpha DB)
bash tools/t137_m4_leg.sh 7836 12 6 750            # expect flips>=1, p99<25ms

# 6. M1 shape: 20 bots x 30 min
./build/linux-gcc/server/bh_server --db /tmp/rf2.db --soak-secs 1800 --p99-budget-ms 10 &
./build/linux-gcc/tools/bots/bh_bots --count 20 --secs 1800 --profile fighter

# 7. macOS: cmake --preset macos-arm64 && build && ctest, then run the client once

# 8. M5: 200 bots x 12 h + 2018 MBA fps (director-scheduled)
```

*Audit executed under `docs/prompts/mvp-playable-audit-2026-09-16.md`. Every
number above came from a command run against HEAD `2bd471d` on 2026-09-16; the
ledger carries the probe for each row.*
