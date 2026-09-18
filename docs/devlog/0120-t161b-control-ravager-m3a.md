# 0120 — T-161b.5 control set + T-161b.6 Ravager set + M3a verdict

## What (item 5, Gravecaller ch15–18, unlocks 4/8/12/10)
- Frost Spike: 5+L+INT + 60t slow (Firebolt mirror, smaller). Slow = 4 steps
  in 5, id-staggered (deterministic, no lockstep).
- Wither: 12 s, stacks to 3, (2+L/4)/stack/20t, caster-credited (wild kills
  pay the void safely — killer-guard spans the pay section). Purify does NOT
  lift rot (flagged: rot is outlasted/outhealed, unlike curse/weakness).
- Terror: 40t rout, bosses immune (the Mother stands), no karma (Weaken law).
  Fear owns the feet (paths cleared, think skips aim/strike).
- Mana Shield: 60 s, 1 MP per 2 dmg; drinks swings/bolts/slams/blasts/pulses.
  Fully-drunk hits read kind 0. Kinds 21/22/23 + cast poses.
- 8 pins (`tests/test_control.cpp`); legs `t161b_control.bwj` (c16=6, c17=1,
  c18=3) + `t161c_control_raider.bwj` (c15=12), both mm=0.

## What (item 6, Ravager ch19–23, unlocks 6/8/12/14/10)
- Sunder (-15% DEF 8 s, arm's reach, stacks 0.85² w/ Weakness), Bull Rush
  (dash ≤3 along path + 40t prone), War Stomp (75% weapon PBAoE r2 + 20t
  stagger, kin + own thrall immune), Execute (AIMED ch22: <20% only, 8 MP,
  100t CD — the passive reading would have doubled legacy swings and cost
  an epoch; flagged), Second Wind (hpMax/4 over 5×20t, any post-mark hit
  breaks it). Prone = no move/no strike on all three paths. powerSwing
  extracted verbatim (ch1 gains prone/ward reads, both neutral). Kinds
  24/25 + cast poses. 6 pins (`tests/test_ravager.cpp`, incl. twin-world
  doubling proof). Leg `t161r_ravager_crypt.bwj`: 15/9/6/3/3, mm=0.

## Neutrality
Every new field unhashed (T-133 law); no RNG draws on old paths; Execute
aimed not passive. All seven prior legs re-replay mm=0 — epoch stays 30
through the entire six-item spine. Suite 370/370 (at droplog; see 0121).

## M3a verdict (partial kit — bots gap found by the numbers)
`logs/t161b_m3_m3e30.bwj` (L13 raider, 900 s): 5/5 map-5 entries
(bossSeen 10–39 each), bossKills 0, curse 46, deaths 88, party=5 held,
replay mm=0. Reach re-proven (better than r22b variance), BUT the new rites
barely fired: the crypt-only support shape never runs on the raider path
(c14/c16–18/c19–23 = 0; only kiter Frost + second-block choir lived).
Fix: mirrored blast/wither/terror/shield + full Ravager set into the
campaign/raider block (shared next*At/counters — mutually exclusive, never
double-casts). M3b re-running with the mirror (`m3e30b`).

## Debt / next
- Close T-161b on M3b (full-kit TTK at the Mother).
- Pet nameplate tint, thrall art (backlog).
- T-159f1.1 (multi-affix ADR+schema) + .2 (tables) batch into the next epoch.
