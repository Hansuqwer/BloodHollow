#include <cstdio>
#include <optional>
#include "world.h"
using namespace bh;
int main() {
  server::World w;
  std::string err;
  if (!w.load("assets/maps/thornwall.bhmap", &err)) { std::printf("load: %s\n", err.c_str()); return 1; }
  if (!w.loadZone(3, "assets/maps/thornwall_crypt.bhmap", &err)) { std::printf("zone3: %s\n", err.c_str()); return 1; }
  server::Entity& p = w.spawn("probe", 0, std::optional<sim::TilePos>{sim::TilePos{55, 19}});  // plaza, SE of chapel door
  const std::uint32_t pid = p.id;
  w.queuePath(*w.find(pid), sim::TilePos{10, 10});
  for (int t = 0; t < 3600; ++t) {
    if (t > 0 && t % 24 == 0) w.queuePath(*w.find(pid), sim::TilePos{10, 10});  // bot resend cadence
    w.tick();
    server::Entity* e = w.find(pid);
    if (e == nullptr) { std::printf("t=%d GONE\n", t); return 2; }
    if (e->zoneId == 3) {
      std::printf("t=%d TRANSFERRED to map 3 at %d:%d\n", t, e->walker.tile().x, e->walker.tile().y);
      return 0;
    }
    if (t % 20 == 0 || (t > 0 && !e->walker.moving && e->path.empty())) {
      std::printf("t=%d pos=%d:%d moving=%d path=%zu zone=%u\n", t,
                  e->walker.tile().x, e->walker.tile().y, e->walker.moving ? 1 : 0,
                  e->path.size(), e->zoneId);
      if (!e->walker.moving && e->path.empty() && t > 0 && t % 24 != 0) {
        std::printf("STUCK settle at %d:%d zone=%u hp=%u\n", e->walker.tile().x, e->walker.tile().y, e->zoneId, e->hp);
        return 3;
      }
      (void)0;
    }
  }
  std::printf("timeout no transfer\n");
  return 4;
}
