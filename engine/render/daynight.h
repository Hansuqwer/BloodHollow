#pragma once

#include <raylib.h>

namespace bh {

// Fullscreen overlay color for a given game-hour [0,24).
// alpha 0 = full daylight; deep blue ramp through night;
// warm band at dawn/dusk. Gameplay effects (light radius etc.) read
// the same hour value — this is the visual half of GDD section 9.
Color nightOverlay(float hour);

}  // namespace bh
