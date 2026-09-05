#include "sim/walker.h"

#include "sim/bhmap.h"  // fnv1a64

namespace bh::sim {

std::uint64_t walkerStateHash(const Walker& w, const std::deque<TilePos>& path) {
  std::uint64_t h = 1469598103934665603ULL;
  auto mix = [&h](std::uint64_t v) {
    for (int i = 0; i < 8; ++i) {
      const std::uint8_t b = static_cast<std::uint8_t>(v >> (8 * i));
      h ^= b;
      h *= 1099511628211ULL;
    }
  };
  mix(static_cast<std::uint32_t>(w.x));
  mix(static_cast<std::uint32_t>(w.y));
  mix(static_cast<std::uint32_t>(w.target.x));
  mix(static_cast<std::uint32_t>(w.target.y));
  mix(w.moving ? 1u : 0u);
  mix(static_cast<std::uint32_t>(w.dir));
  mix(static_cast<std::uint32_t>(path.size()));
  for (const TilePos& t : path) {
    mix(static_cast<std::uint32_t>(t.x));
    mix(static_cast<std::uint32_t>(t.y));
  }
  return h;
}

}  // namespace bh::sim
