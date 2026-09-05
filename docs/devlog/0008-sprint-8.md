# Devlog 0008 — Sprint 8: the hatch opens both ways *(2026-09-04)*

Eighth sprint: Phase 3 lift-off — zones-in-process — plus a toolchain rescue.

## Delivered

- **Zones-in-process (T-036, ADR-0003 honored)** — `World` now holds
  `unordered_map<mapId, Zone>` (map+costgrid+spatial+spawners+spawnPoint each),
  entities carry `zoneId`, all pathing/AoI/combat/vendoring resolve through
  `zoneOf(e)`. Boot auto-loads `thornwall_crypt.bhmap` as map 3 when present.
  Cross-zone targeting is impossible; per-zone spawners tick independently.
- **Portals** — tile-rect triggers fire for settled players; 40-tick arrival
  grace kills ping-ponging. Live evidence: 5+ real `[zone] scout -> map 3`
  handoffs across gates; worldHash mixes zoneId.
- **Client handoff (T-037)** — `Welcome` doubles as ZoneTransfer (server
  resends + interest-table reset), client hot-swaps the bhmap via
  `Game::zoneReload`, clears remote ents/floaters/targeting, camera snaps to
  the arrival tile. Bots reset on re-Welcome the same way and kept moving in
  the crypt (SUMMARY minDeltas stayed full across transfers).
- **Persist v4 (T-039)** — `characters.map_id`, saved on transfer AND on
  logout; login spawns into the persisted zone (verified: two scouts rejoin
  inside the crypt at their exact transfer tiles).
- **Journal fix** — pump-phase events (login/connect) now recorded at the first
  tick they affect (was off by one); soak-shutdown disconnects journaled
  explicitly (disconnect_now fires no event). Post-fix: **crossing-inclusive
  session replays 17/17 hashes bit-exact** (`logs/2026-09-04-zones-*.bwj/txt`).
- **tools/bootstrap.sh** — snapshots don't persist `build/` or layer tools;
  one script restores cmake + X11/GL headers/libs from debs and rebuilds green.
  (This turn cost it proving itself immediately: full rebuild, 43/43 green.)

## Verification

- Tests **43 cases / 327,146 assertions green** (new: one-way + round-trip
  portal transfer with state inventory/gold carried; AoI zone isolation).
- Two-zone 300 s gate with 5 grinders + 3 scouts: p99 0.43 ms, both zones
  ticking mobs (88 entities peak), zero disconnects, replay verified.

## Landmark reached

You can now walk through the chapel hatch and the world *remembers you there*.
Next: crypt GM pass (palette + a scripted delve), then T-038 (the Anvil/aura
spine doc — Soma's weapon tiers as Bloodhollow quests), and the M2b L1->8
campaign run across both zones.
