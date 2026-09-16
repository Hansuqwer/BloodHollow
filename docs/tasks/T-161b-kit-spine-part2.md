# T-161b — Kit spine part 2 (filed 2026-09-16, after T-161 Resurrect)

**Status:** `open` — wave-2 shipped ch10 Resurrect; this card owns the rest of the T-161 priority order.

## Scope (in order, each its own commit)

1. **Sanctuary** (ch11): ground healing circle, 600t CD, radius + tick heal, decal-visible (T-ART-09 exists), no stacking.
2. **Curse of Weakness** (ch12): −15% target dmg/def + Purify interaction (one law, both ways).
3. **Raise Skeleton** (pet entity: new kind, leash, despawn law — largest).
4. **Corpse Explosion** (needs corpse entity/decay law).
5. Gravecaller control set (Frost Spike, Wither, Terror, Mana Shield).
6. Ravager set (Sunder, Bull Rush, War Stomp, Executioner, Second Wind).

Rules carried over: server-authoritative range/CD/MP/legality, journaled via existing `kSkill` channels (no new kinds unless a mechanic needs it), RNG via `sim/rng.h`, unit pins + parity notes per skill, client hotkey scheme extended (7/8 reserved), help panel updated, GDD §3 rows flipped as each ships. Epoch bump only if sim semantics move (coordinate — one bump per wave).

## Acceptance (per batch)

Unit pins per skill, fresh leg firing every new skill at least once → mm=0, M3 party-vs-solo re-measured with the fuller kit (feeds T-157).
