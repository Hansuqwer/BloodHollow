# 0017 — anvil gear-churn (Sprint 16): wear it, name it, tempt it

Three verbs landed together because they only read as one sentence:
**the gear in Bloodhollow is never settled.**

## What shipped

- **T-058 durability.** Weapons burn 1 point per landed swing; armor burns
  1 per hit taken. At zero the piece goes *dormant* — grey in the pack,
  fists/bare-skin in the math, never destroyed. Marta repairs for
  half-a-point-per-gold, all or nothing, and only if the law lets you in
  her door.
- **T-059 affixes v1.** Three one-liner mods rolled at drop time from a
  per-mob side table (Ghoul shingles 4% Rusty Shank, …): *of Whet*
  (+10% dmg), *of Warding* (+2 def), *of Leech* (sip 5% of what you deal).
  Affixes ride silent while the piece is dormant.
- **T-060 refine.** At the Widow Anvil: one monster part + 50 gold per
  knock on the coal. First two tiers are guaranteed (era mercy); the third
  is a 60/40 coin — lose it and the item **shatters**. +2 attack (weapon) /
  +1 DEF (armor) per tier.

Craft arithmetic order is pinned in code comments: refine adds run first,
then the whet multiplier, then aura flats — so a +III Whet blade stacks
honestly instead of double-dipping.

## The one rule we kept from Soma

Soma's anvil risked the item. We kept the risk and refused the lottery-skin:
guaranteed floors first, coin flip only on the last coal. A poor player
climbs to +2 safely; a rich one gambles for +3. That asymmetry *(certainty
is cheap, certainty+ is expensive)* is the whole moral economy in miniature.

## Gates

- Unit: 7 new gear-churn cases; suite **83/83** (327,466 assertions).
- Determinism: new kill-time gear rolls push the journal epoch **3 → 4**
  (documented chain at `kJournalEpoch`); S16 smoke leg records
  `v 4`, replays **bit-exact** — `ticks=1455 sessionCmds=208 hashes=14
  mismatches=0 entities=290`. Bots used `/repair` 14 times unprompted
  (`mend=14`), which is exactly the loop telemetry we wanted.
- Legacy S13–S15 journals now legitimately refuse with exit 4 — the "stale
  epoch, not a broken build" contract working as designed.

## Next (Sprint 17, queued on the board)

- **T-061** night buffs / aura swaps (mob side).
- **T-062** +25% drops, +10% XP at night with the legibility floor (caps
  darkness so the dark is a promise, not an eyepatch — the private-server
  "permanent day" lesson).
