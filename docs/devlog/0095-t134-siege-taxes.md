# Devlog 0095 — T-134 siege taxes + vault + holder buff (Phase S, 3/4)

## What

The castle economy: `siege_state` single-row table (holder, vault,
crowns) loaded at boot and throttled-saved (≥600 ticks) on a dirty flag
from `tickServer`. While a holder stands, 5% of every PvE kill-gold award
(post-Greed/post-moral, silent) accrues to the tax-only vault; the holder
entity acts blessed (+10% hit&dmg, holder-id gated). Crown completion
books name/crowns/dirty. `gm siege` directed readout (holder/vault/crowns/
battle/bands). Vault spending integrates with the pledge vault in Phase P.

## Epoch

NONE (stays 24): zero draws; `t128.bwj` re-replays mm=0.

## Evidence

- `test_siege_taxes.cpp`: 6 cases — twin-world tithe math (hound floor
  keeps it non-trivial), buff on/off, Db round-trip + empty default,
  boot-load + readout, crown booking, 600-tick throttle (suppress +
  re-due).
- Full ctest 2/2 (269 cases). validate_links 0/5. Duel pin unchanged.

## Next

S4 (T-135): siege bot behavior + rehearsal harness + M4 gate (40 bots,
p99 <25 ms, 3/3 flips — the full-window leg runs background).
