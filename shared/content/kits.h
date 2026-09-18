#pragma once

#include <cstdint>

namespace bh::content {

// Class kits (T-053/T-054, GDD §3). Content, not engine; server, client and
// bots share the table. ids are stable across the wire, the DB column
// (characters.class_id) and the journal (kKitChoose.a).
enum : std::uint8_t {
  kKitUnsworn = 0,
  kKitRavager = 1,
  kKitGravecaller = 2,
  kKitCultist = 3,
};

struct KitDef {
  std::uint8_t kitId;
  const char* name;       // HUD label ("Pale Choir" is the fiction name)
  std::uint8_t str, vit, dex, intg, mag;  // creation seed
  // unlock level per skill channel (0 = kit may not channel it):
  // ch1 Power Swing, ch2 Mend, ch3 Bless, ch4 Ironskin, ch5 Firebolt,
  // ch6 Chorus, ch7 Mass Mend, ch8 Haste (T-054b kit-v2 channels),
  // ch9 Purify (T-082 field cleanse), ch10 Resurrect (T-161 support spine),
  // ch11 Sanctuary (T-161b.1 ground hold), ch12 Curse of Weakness (T-161b.2),
  // ch13 Raise Skeleton (T-161b.3 thrall), ch14 Corpse Explosion (T-161b.4),
  // ch15 Frost Spike, ch16 Wither, ch17 Terror, ch18 Mana Shield (T-161b.5),
  // ch19 Sunder, ch20 Bull Rush, ch21 War Stomp, ch22 Execute, ch23 Second
  // Wind (T-161b.6 Ravager set).
  // Width 24: content-only widening — unlocks are looked up at runtime,
  // never serialized, so no wire/DB impact (T-161 cleared the caution).
  std::uint8_t chUnlock[24];
};

// Seeds sum to the legacy uniform 24 pool (era: no free power for picking).
// Ravager keeps the exact legacy seed 8/8/8/0/0 so every pre-kit character
// (and the campaign bot baseline) is behavior-identical after migration.
inline constexpr KitDef kKits[] = {
    // Ravager: Power Swing only (v1). Cultist keeps ch1 for self-defense
    // between casts; Gravecaller cannot swing it (squish tax for nuke range).
    // T-161b.6 Ravager set (all flagged): ch19 Sunder 6, ch20 Bull Rush 8,
    // ch21 War Stomp 12, ch22 Execute 14, ch23 Second Wind 10.
    {kKitRavager, "Ravager", 8, 8, 8, 0, 0, {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 8, 12, 14, 10}},
    // T-161b.4: Gravecaller ch14 Corpse Explosion unlocks at 14 (flagged:
    // needs corpses = group play — arrives with the Crypt-tier kit).
    // T-161b.5 control set (all flagged): ch15 Frost Spike 4 (early shape
    // beside Firebolt 1), ch16 Wither 8, ch17 Terror 12, ch18 Mana Shield 10.
    {kKitGravecaller, "Gravecaller", 6, 6, 6, 4, 2, {0, 0, 0, 0, 0, 1, 0, 0, 10, 0, 0, 0, 0, 0, 14, 4, 8, 12, 10, 0, 0, 0, 0, 0}},
    // T-082: Cultist ch9 Purify unlocks at 6 (utility-tier parity with
    // Ironskin — flagged derivation, not GDD text).
    // T-161: Cultist ch10 Resurrect unlocks at 20 (GDD §3 pillar skill).
    // T-161b.1: ch11 Sanctuary unlocks at 14 (flagged: between Mass Mend 12
    // and Resurrect 20 — the hold arrives when parties start living in it).
    // T-161b.2: ch12 Curse of Weakness unlocks at 12 (flagged: Mass-Mend
    // tier — the debuff arrives with the party kit, not before it).
    // T-161b.3: ch13 Raise Skeleton unlocks at 16 (flagged: Crypt-tier
    // parties — a tank-thrall at 12 would trivialize the Mine bands).
    {kKitCultist, "Pale Choir", 5, 8, 5, 3, 3, {0, 1, 1, 3, 6, 0, 9, 12, 11, 6, 20, 14, 12, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

inline const KitDef* findKit(std::uint8_t kitId) {
  for (const auto& k : kKits)
    if (k.kitId == kitId) return &k;
  return nullptr;
}

inline std::uint8_t kitSkillUnlock(std::uint8_t kitId, std::uint8_t channel) {
  if (channel >= 24) return 0;
  const KitDef* k = findKit(kitId);
  return k != nullptr ? k->chUnlock[channel] : 0;
}

}  // namespace bh::content
