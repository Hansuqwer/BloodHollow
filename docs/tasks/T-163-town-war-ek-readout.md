# T-163 — Town war reachable + player-visible EK readout (P1, audit 14/20)

## Context
Audit findings **B1.5 / B8.6 / B8.7 / C11**. The town-war spine ships: oath at
L19 (`kTownOathLevel`, `/oath`), Thornwall/Marrowgate identities
(`shared/content/towns.h`), EK fame persisted (`characters.ek`), guard kinds
70/71, and a readout — but the readout is **`gm ek`**, a GM chat verb
(`server/src/main.cpp:635`), and there is **no Marrowgate population**: no zone,
no mobs, no NPCs carrying that identity beyond one guard kind, so a player can
swear and then never meet an enemy. MVP §1 keeps Marrowgate OUT as a city sim but
IN as "red-named EK targets in the field war". Today the field war has no targets.

## Scope
- **Make the war reachable without building a second city**: Marrowgate-affiliated
  presence in the existing field/mine zones — e.g. a named-elite patrol or a
  warband spawner carrying `kTownMarrowgate` whose kills grant EK to sworn
  Thornwall players (and vice versa if cheap), plus red-name treatment so the PK
  law reads correctly. Keep it to content + one spawner law; no new zone.
- **Player EK surface**: `/ek` (self + top-N board) as a player verb reusing the
  existing `ekReadout` text, and — if T-151's wire lands first — a small board
  panel. `gm ek` stays for operators.
- **Sink**: teleport scroll item + a town-portal sink if the director wants the
  GDD §7 economy row honoured (audit C11); otherwise record the cut in T-158.
- OUT: a playable Marrowgate city, nation war/Crusade, EK web page (director card
  T-147), season resets.

## Acceptance criteria
1. A sworn Thornwall character can earn EK from an NPC-affiliated enemy without
   another human present (live leg: `/oath` → kill → `ek` column increments).
2. `/ek` returns self + top-N for any player; `gm ek` unchanged; unit pins for
   both paths and for the unsworn refusal.
3. EK persists across relog (sqlite before/after) and is not farmable from
   lawful kills (karma law pin).
4. Replay `mm=0`; epoch bump only if spawner composition changed (likely — say so).
5. GDD §1/§5 amended or ADR'd for whatever the field-war shape became (T-158).

## Tests required
EK law pins (sworn/unsworn, lawful vs enemy-kill), readout verb pins, spawner pin.

## Evidence owed at merge
Suite count, leg log with the EK increments, screenshot of `/ek`, devlog, board row.
