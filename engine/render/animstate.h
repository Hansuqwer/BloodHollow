#pragma once

#include <cstdint>

// T-ART-04 client anim-state hook: render-side combat animation selection.
// The client learns about swings only when the server's CombatEvent pulse
// arrives (post-resolution) — so the attack/cast anim STARTS on the contact
// frame, which is exactly the backlog's timing truth ("contact frame lands
// on the swing resolution tick"). No prediction, no authority: durations
// tick down in wall time (20 Hz domain) and missing anims fall back to
// walk/idle at the draw call (animFrame returns empty when absent).
// Raylib-free so the unit suite can pin it.
namespace bh {

// Render-side combat states (priority at set time: die > hurt > cast/attack).
enum class EntAnimState : std::uint8_t { kNone = 0, kAttack, kCast, kHurt, kDie };

// Transient-state durations, in sim ticks (rendered as ticks/20 s). Attack
// is half the player swing cadence (kPlayerAtkCdTicks = 16) so a full swing
// always finishes before the next begins; hurt is a flinch; die holds until
// despawn (no timer — the corpse beat belongs to the vestige layer).
inline constexpr int kAttackAnimTicks = 8;
inline constexpr int kCastAnimTicks = 8;
inline constexpr int kHurtAnimTicks = 4;

inline int animStateDurationTicks(EntAnimState st) {
  switch (st) {
    case EntAnimState::kAttack: return kAttackAnimTicks;
    case EntAnimState::kCast: return kCastAnimTicks;
    case EntAnimState::kHurt: return kHurtAnimTicks;
    case EntAnimState::kDie: return -1;  // held, not timed
    case EntAnimState::kNone: return 0;
  }
  return 0;
}

// Atlas anim name for a state; walk/idle when quiet. The draw call falls
// back to these when the atlas lacks the combat anim.
inline const char* animNameFor(EntAnimState st, bool moving) {
  switch (st) {
    case EntAnimState::kAttack: return "attack";
    case EntAnimState::kCast: return "cast";
    case EntAnimState::kHurt: return "hurt";
    case EntAnimState::kDie: return "die";
    case EntAnimState::kNone: return moving ? "walk" : "idle";
  }
  return "idle";
}

}  // namespace bh
