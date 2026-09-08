# 0033 — Blood Curse + chapel cure: the bolt leaves thin blood (T-070)

S22a of the overnight queue (day/night completion, first of two cards).

## Scope as pinned

- `Entity.curseUntil` (buff-stamp precedent). The T-064 boss-bolt cast sets
  `curseUntil = tick + 600` (30 s) on a player hit. Potions (`useItem`) and
  Mend (`tryMend` via `trySkill` chan 2) heal **75%** while
  `tick_ < curseUntil`; OOC regen untouched (lines verified, not modified).
- Confessor furniture (`kWireKindConfessor = 68`), seeded once in the chapel
  rect (9,9)–(14,13) by `World::spawnConfessor` (first walkable
  non-furniture tile). `/confess` → `kConfess` (journaled, appended at the
  enum end so old journal kind-ints are untouched) clears within 3 tiles
  with a fiction line; quiet fail when clean or far.
- Client: violet `CURSE -25% heal Ns` second buff row from
  `OwnStats.curseTicksLeft`; confessor renders via the generic furniture
  rect + name label (no new art).
- Epoch **8 → 9** + fresh-leg replay.

## Premise corrections (brief vs tree — director review)

1. **Gravecaller has no bolt.** The brief names "the Gravecaller Blood Bolt
   (T-064) and the Gravemother bolt" as joint sources; in this tree only
   the Gravemother (1009: boss=1, boltRange=6, cd 26) carries the T-064 kit
   — Gravecaller (1007) is boss=0/boltRange=0 and never casts (client
   comment at game.cpp:416 agrees: "Gravemother's ranged cast"). Giving
   1007 a bolt would be a mob retune = design decision = off-limits. The
   source is therefore **the shared boss-bolt path** (any future bolt boss
   inherits the curse); curse content is L14-crypt-adjacent by construction.
2. **Bless is not scaled.** The brief's "potions, Mend, and Bless heal for
   75%" assumes a Bless heal that does not exist — Bless is +10% hit&dmg
   with no heal component. Scaling nothing is not a nerf; flagged, not
   implemented.
3. **vfx kind-14 is not free.** The brief reserves it for a violet-eye
   debuff; the tree has it as HASTE amber. No new CombatEvent kind was
   added — the curse shows in the buff row + a sys line. Wire stays minimal.
4. **Wire version.** `OwnStats` grows one trailing `u16 curseTicksLeft`
   (messages.md documents it). `kProtocolVersion` stays 237 by the
   200+message-count law — same class as S16's ItemSlot growth, which also
   added fields without a bump; fleet deploys lockstep (server+bots+client
   from one tree). Stated, not silent.

## Soak + replay

Fresh 580 s grinder mix (port 7871, fresh DB `/tmp/t070.db`, epoch-9
journal `logs/t070.bwj`), same 14-bot shape (5 wander / 4 fighter /
3 pilgrim / 2 campaign):

| group | kills | deaths | maxLevel | notes |
|---|---|---|---|---|
| campaign ×2 | 51 | **0** | 3 | killerByLvl empty; mend=11, bless=2 live |
| fighter ×4 | 143 | 52 | 4 | spread L2–L11 |
| pilgrim ×3 | 120 | 48 | 4 | spread incl. L9 widow sightings |
| wander ×5 | 0 | 16 | 1 | decoys |

Bands: Rat 4.0 / Bat 2.3 / Ghoul 11.3 / Hound 23.1 / Gnoll 111.4 (n=9) /
Gravecaller 274.6 (n=4) — ordering preserved; mid-band wobble continues
the T-069 watch item (Ghoul 13.9→11.3 drifting back; noise, not a trend).
Entities 319–329, p99 ~4.0–5.4 ms.

`./build/server/bh_server --replay-world logs/t070.bwj` →

`[replay] OK ticks=13601 sessionCmds=6093 hashes=137 mismatches=0 entities=342`

## Files

`server/src/{world.cpp,world.h,command.h,main.cpp}`,
`shared/content/wirekind.h`, `shared/protocol/messages.md`,
`client/src/{game.cpp,net_client.cpp,net_client.h}`,
`tests/{test_curse.cpp,CMakeLists.txt}`. Suite **119 / 328,008**,
ctest 2/2, warning-free. No mapgen touch (confessor is server-seeded).
