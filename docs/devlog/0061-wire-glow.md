# 0061 — The wire grows a glow (T-092)

T-092 closes the T-ART-11 remainder: refine glow now renders in-world, not just in the inventory panel.

## What landed

- Two lines in `messages.md`, three pack sites, two unpack sites, one halo. The whole card is plumbing — the design (tiers, alpha cap, dormant-wins) was already pinned by T-ART-11 on both ends, so the only new decision was *where the byte rides*: trailing on spawn + delta, the same lane the T-071 light byte took. Refine swaps and repairs propagate live because the delta carries it every tick.
- One build red during the sprint: `equippedGlowTier` landed in the private block while the net layer in `main.cpp` is a free function. Promoted to public next to `karmaBandOf` — same rationale (const query the wire needs), no wrapper games.

## Judgment calls

- No epoch bump, proven not argued: the epoch-13 soak journal replays byte-clean on the new binary. A presentation-only field derived from hashed state cannot perturb the hashes, and now there is a replay line saying so.
- No version bump, per the count law: two trailing `u8`s are exactly the S16/ItemSlot, T-070/OwnStats, T-071/Spawn-Delta precedent class. The strict-Reader risk (deserialize fails closed on truncation) is covered by lockstep deploys plus the live smoke: real bots conversed, moved, killed, and replayed with the new bytes in flight.
- Halo ellipse, not sprite overlays: the backlog's "~2 overlay frames" await art. The halo obeys the same alpha law, so the swap later is a draw-call change, not a re-pin.
