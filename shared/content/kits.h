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
  // ch6 Chorus, ch7 Mass Mend, ch8 Haste (T-054b kit-v2 channels)
  std::uint8_t chUnlock[9];
};

// Seeds sum to the legacy uniform 24 pool (era: no free power for picking).
// Ravager keeps the exact legacy seed 8/8/8/0/0 so every pre-kit character
// (and the campaign bot baseline) is behavior-identical after migration.
inline constexpr KitDef kKits[] = {
    // Ravager: Power Swing only (v1). Cultist keeps ch1 for self-defense
    // between casts; Gravecaller cannot swing it (squish tax for nuke range).
    {kKitRavager, "Ravager", 8, 8, 8, 0, 0, {0, 1, 0, 0, 0, 0, 0, 0, 0}},
    {kKitGravecaller, "Gravecaller", 6, 6, 6, 4, 2, {0, 0, 0, 0, 0, 1, 0, 0, 10}},
    {kKitCultist, "Pale Choir", 5, 8, 5, 3, 3, {0, 1, 1, 3, 6, 0, 9, 12, 11}},
};

inline const KitDef* findKit(std::uint8_t kitId) {
  for (const auto& k : kKits)
    if (k.kitId == kitId) return &k;
  return nullptr;
}

inline std::uint8_t kitSkillUnlock(std::uint8_t kitId, std::uint8_t channel) {
  if (channel >= 9) return 0;
  const KitDef* k = findKit(kitId);
  return k != nullptr ? k->chUnlock[channel] : 0;
}

}  // namespace bh::content
