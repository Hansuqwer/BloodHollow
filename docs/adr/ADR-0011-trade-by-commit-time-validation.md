# ADR-0011 — Trade via commit-time validation (no escrow state machine)

**Status**: accepted (S7) **Context**: two-player trade must never duplicate or
lose items even with disconnects mid-negotiation. Classic escrow (server holds
offered items immediately) needs refund paths for every abort reason.

**Decision**: offers are *declarative intents* stored on each Entity
(itemId+qty list, gold). Nothing is deducted until BOTH players commit. At the
commit tick the world validates distance, life state, ownership, quantities and
gold for both sides; failure cancels cleanly (nothing ever moved), success swaps
in one tick inside the single-threaded sim loop. Movement >3 tiles, death, or
partner absence auto-cancels in the same tick pass.

**Consequences**: (+) impossible to dupe/refund-miss; no escrow bookkeeping;
rollback = early-exit. (+) unit-testable wholly in World. (-) offers can be
"spite-committed" by griefers offering items they no longer own → commit simply
cancels with a system line; acceptable for MVP. (-) offers are invisible to the
partner except via directed chat lines; richer two-column window = Sprint 8+.

**Alternatives rejected**: escrow-with-refunds (state machine bug surface);
sim-level item locks (blocks equip/use interplay testing).
