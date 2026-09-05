#pragma once

#include <raylib.h>

#include "assets/atlas.h"

namespace bh {

// Procedural placeholder art: the repo ships zero binary assets (ADR-007
// pipeline lands in Sprint 2). A hooded little figure, 8 dirs x (6 walk
// frames + 1 idle), 32x48 per frame, drawn pixel-style at startup.
Atlas makeHeroAtlas(Color body, Color trim, Color skin);

}  // namespace bh
