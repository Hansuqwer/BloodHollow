# 0015 — Swear the oath, keep the sword arm

Sprint 14, 2026-09-05. Cards: T-053 (class kits foundation), T-054 (Cultist
kit v1 + Gravecaller starter), T-055 (choir bot + dual-kit gate leg). Phase 3
continues from the party spine.

## The freeze rule

Every balance ship since S8 sailed on 8/8/8 characters who are, in the new
fiction, Ravagers. So the kit pass's first law is **the default kit is a
no-op**: `classId` defaults to 1 (Ravager), the Ravager seed is exactly the
legacy 8/8/8/0/0, and old journals replay the same hashes on the new build.
Everything else is opt-in content on top of a frozen baseline.

The GDD sums to 24 points; kits skew them. Gravecaller (6/6/6/INT4/MAG2) pays
body for plague-fire. The Pale Choir (5/8/5/INT3/MAG3) is deliberately the
tanky caster — Soma's "support has to survive the pull it enables". Seeds,
not multipliers; the +3/level assign points stay player-owned on top.

## The oath

`/kit cultist` — one word, sworn once (re-dedication window at level 1, hard
stop at level 2+; the journal records a `kKitChoose` command with the kit id,
so oath-swearing is replay-exact like everything else). DB schema v7 adds
`characters.class_id DEFAULT 1`. New wire fields on OwnStats: classId, mp,
mpMax, buff countdowns (v237 — field growth doesn't tick the count-derived
version number, same convention as inventory slots).

Starting points aren't the design; **lists are** — the dispatch table says who
may channel what, when:

| channel | skill | Ravager | Gravecaller | Cultist |
|---|---|---|---|---|
| 1 | Power Swing | L1 | — | L1 (self-defense) |
| 2 | Mend | — | — | L1 |
| 3 | Bless | — | — | L3 |
| 4 | Ironskin | — | — | L6 |
| 5 | Firebolt | — | L1 | — |

## The Choir (T-054)

Mend: 30+4·level to self or a party member within 6 tiles, 8 MP, 25-tick
cooldown. Heals refuse mobs, refuse strangers, refuse the overextended (era
triangulation: support heals the sworn circle, not the world). Bless: +10%
hit & damage for 5 minutes, party-target. Ironskin: +20% mitigation for 5
minutes. Both buffs are stack-free refresh stamps (`blessUntil`,
`ironskinUntil`); the +10% sits in the shared `effAcc/effDmgBase` path so
autoattacks AND Power Swing both drink, and `effDef` feeds hits taken the
other way. No buff vector, no dispel system — two stamps until Chorus.

Firebolt rides in for free (dispatch proof north-to-south): ranged 8-tile
nuke, `8 + 2·level + 2·intg`, ignores armor (plague-fire isn't steel), 0.65
PvP scalar, 6 MP, 30-tick CD. Gravecallers cannot power-swing. That's the
squish tax and it's on-purpose.

MP is new but deliberately anemic: pool 30+6/level, regen 1/s, ding refill.
It's a pacing gauge, not an economy — MAG-as-stat and real mana pressure
arrive with the spell-circle sprint.

## The sinkhole debt

A mid-patch include fusion and a brace-eating struct decl ate ten build
minutes this sprint; doctest's no-`||`-in-CHECK rule caught a sloppy
two-valued kill assertion. All three classes of error mechanical, all three
caught inside the build loop. Also discovered (again) that the sandbox
evicts `~/.local` and `/tmp` — cmake and the X11 dev-bits vanished mid-sprint;
`tools/bootstrap.sh` did its job and got the toolchain back in one command.
Memory aid: **that script exists, next reset just run it first.**

## Gate (T-055)

Dual-kit leg: Ravager leader + Cultist choir-bot (auto-swears, auto-parties
with the race-proofed formation loop — welcome-gated invites, 10 s retry
cadence until roster size 2 confirms), mends anyone under 60%, blesses the
leader on rotation. Benchmark = S13 two-Ravager leg, same knob settings.

| moment | S13 two-Ravager | S14 Ravager+Choir |
|---|---|---|
| L2 | 22 / 28 s | 26 / 33 s |
| L3 | 179 / 226 s | **92 / 145 s** |
| L4 | 228 / 326 s | 426 / 475 s |
| L5 | 556 / 583 s | 594 s (cultist: L4, soaked out) |
| deaths | 36 | 38 (22 Rav / 16 Choir) |
| kills | 136 | 138 |
| party casts | — | 77 Mends, 3 Blesses |
| replay | OK (98 hashes, 0 mismatch) | **OK (114 hashes, 0 mismatch)** |

**Reading, honestly:** the choir buys *early* velocity (L3 nearly halves —
61-84 healed hits before the first shop run) and the replay discipline holds
with buffs in flight. It does not yet buy *survival* at band — deaths are a
wash — and the late L4→L5 leg drags: the cultist's swing is a ravager's minus
17% (STR 5+small stat grow), one Mend per 1.25 s is 27 hp/s next to ghoul
burst, and Bless sat at ~1/10 uptime on one target. The GDD claim ("party
with a Cultist out-farms/out-survives one without") is a **Level-3-6 claim
answered 'mixed'**, not yet the L8 claim. Expected levers, in order: Chorus
(party-wide buffs) + Mass Mend at their GDD levels, Haste for rotation, and
the T-034b camp retune so ghoul spikes stop eating the heal window whole.

Also caught by the gate evidence this sprint, fixed same-day:
- **Party formation race** (bot.name is pre-seeded pre-login — useless as an
  online signal; the invite would resolve to say-text and the pair never
  formed). Bot pair-up now welcome-gated with 10 s retries.
- **Same-tick accept-before-invite ordering** — solved by the retry loop:
  invites renew; an accept inside the next 200-tick window lands.
- **doctest/printf/vformat hygiene**: `./~/.local` and `/tmp` evicted
  mid-sprint cost one toolchain rebuild; bootstrap covered it.

## Next

Alignment/PK rules (karma is data; now it needs teeth), anvil gear-churn
(durability/refine/affixes), day/night gameplay, Bonehowl Mine + Dropped Crypt
+ Gravemother + elites + bounty board, VFX/audio pass 1. T-034b camp retune
and the M2b-final L1→8 party rerun hang off the same branch.
