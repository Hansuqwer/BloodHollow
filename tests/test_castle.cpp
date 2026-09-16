// T-123 Weeping Castle (mapId 6): the siege stage, pinned at asset level.
// B3 ships geometry + furniture + the dead garrison; the siege LAW (gate
// damage type, crown channel, ownership flips, scheduler) is the B4 card.
// These pin what B4 will build on:
//   - zone 6 loads and the portal graph closes (fields <-> castle)
//   - the four fixed pieces exist with GDD §8 HP pools at their mapgen tiles
//   - the garrison spawners register and fill (sentinels + moor hounds)
//   - both causeways + gate gaps are walkable from the moor to the heart
//   - the Oathbroken Sentinel exists in the content table (L14, no boss kit)
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

#include "content/mobs.h"
#include "sim/bhmap.h"
#include "world.h"

using namespace bh;

namespace {
bool hasPortalTo(const sim::Map* m, std::uint16_t target) {
  if (m == nullptr) return false;
  for (const auto& pd : m->portals)
    if (pd.targetMapId == target) return true;
  return false;
}

server::Entity* findPiece(server::World& w, std::uint8_t wireKind, int x, int y) {
  for (auto& e : w.entities())
    if (e.wireKind == wireKind && e.zoneId == 6 && !e.dead &&
        e.walker.tile().x == x && e.walker.tile().y == y)
      return &e;
  return nullptr;
}
}  // namespace

TEST_CASE("T-123: the castle loads and the portal graph closes (asset gate)") {
  server::World w;
  std::string err;
  REQUIRE(w.loadZone(2, "assets/maps/fields_overflow.bhmap", &err));
  REQUIRE(w.loadZone(6, "assets/maps/weeping_castle.bhmap", &err));
  const sim::Map* fields = w.zoneMap(2);
  const sim::Map* castle = w.zoneMap(6);
  REQUIRE(fields != nullptr);
  REQUIRE(castle != nullptr);
  CHECK(castle->w == 56);
  CHECK(castle->h == 44);
  CHECK(hasPortalTo(fields, 6));  // castle_road_in (east edge)
  CHECK(hasPortalTo(castle, 2));  // castle_road_out (moor road)
}

TEST_CASE("T-123: the four fixed pieces stand on their mapgen tiles") {
  server::World w;
  std::string err;
  REQUIRE(w.loadZone(6, "assets/maps/weeping_castle.bhmap", &err));

  const server::Entity* wg = findPiece(w, content::kWireKindCastleGate, 20, 34);
  const server::Entity* eg = findPiece(w, content::kWireKindCastleGate, 36, 34);
  const server::Entity* hs = findPiece(w, content::kWireKindHeartstone, 28, 20);
  const server::Entity* th = findPiece(w, content::kWireKindThrone, 28, 11);
  REQUIRE(wg != nullptr);
  REQUIRE(eg != nullptr);
  REQUIRE(hs != nullptr);
  REQUIRE(th != nullptr);

  // GDD §8: gates 100k HP (the x3 siege-damage law is B4)
  CHECK(wg->name == "West Gate");
  CHECK(eg->name == "East Gate");
  CHECK(wg->hpMax == server::World::kCastleGateHp);
  CHECK(eg->hpMax == server::World::kCastleGateHp);
  CHECK(wg->hp == server::World::kCastleGateHp);
  CHECK(eg->hp == server::World::kCastleGateHp);
  // heartstone: B4-tunable stand-in (GDD prices neither it nor the toll)
  CHECK(hs->name == "Heartstone");
  CHECK(hs->hpMax == server::World::kCastleHeartHp);
  // the throne is furniture: the channel reads its occupant, not its HP
  CHECK(th->name == "Weeping Throne");
  CHECK(th->hpMax == 1);

  // exactly two gates, exactly one heart, exactly one throne
  int gates = 0, hearts = 0, thrones = 0;
  for (const auto& e : w.entities()) {
    if (e.zoneId != 6 || e.dead) continue;
    if (e.wireKind == content::kWireKindCastleGate) ++gates;
    if (e.wireKind == content::kWireKindHeartstone) ++hearts;
    if (e.wireKind == content::kWireKindThrone) ++thrones;
  }
  CHECK(gates == 2);
  CHECK(hearts == 1);
  CHECK(thrones == 1);
}

TEST_CASE("T-123: the dead garrison holds the stage") {
  server::World w;
  std::string err;
  REQUIRE(w.loadZone(6, "assets/maps/weeping_castle.bhmap", &err));
  // loadZoneFrom runs initialMobSpawns: 4 sentinel spawners (maxAlive 1)
  // + the moor hound pack (maxAlive 2) must be standing
  int sentinels = 0, hounds = 0;
  for (const auto& e : w.entities()) {
    if (e.zoneId != 6 || e.dead || e.kind != server::EntityKind::kMob) continue;
    if (e.wireKind >= content::kWireKindFurnitureFloor) continue;  // furniture
    if (e.name == "Oathbroken Sentinel") ++sentinels;
    if (e.name == "Hollow Hound") ++hounds;
  }
  CHECK(sentinels == 4);
  CHECK(hounds == 2);

  // content table: L14 defender, no boss kit (crowd law stays with bosses)
  const content::MobDef* md = content::findMob(1015);
  REQUIRE(md != nullptr);
  CHECK(md->level == 14);
  CHECK(md->boss == 0);
  CHECK(md->guard == 0);  // dead garrison aggroes anyone, not just the wanted
}

TEST_CASE("T-123: both gates are breachable paths from the moor to the heart") {
  server::World w;
  std::string err;
  REQUIRE(w.loadZone(6, "assets/maps/weeping_castle.bhmap", &err));
  const sim::Map* m = w.zoneMap(6);
  REQUIRE(m != nullptr);
  // the stage must be walkable: portal landing (2,40) -> each causeway ->
  // each gate gap -> the heart crossroads (28,20) -> the throne door (28,13)
  // (straight-line walkability of the carved tiles; A* runs the same grid)
  const int road[][2] = {{2, 40}, {2, 38}, {12, 38}, {19, 38}, {20, 36},
                         {20, 35}, {20, 34}, {21, 33}, {27, 33}, {28, 21}};
  for (const auto& xy : road) {
    if (!m->inBounds(xy[0], xy[1])) {
      FAIL("road tile out of bounds: ", xy[0], ",", xy[1]);
    }
    if (m->isBlocked(xy[0], xy[1])) {
      FAIL("road tile blocked: ", xy[0], ",", xy[1]);
    }
  }
  // east causeway mirrors west: gate gap (36,34) over the moat (36,35)
  CHECK_FALSE(m->isBlocked(36, 35));
  CHECK_FALSE(m->isBlocked(36, 34));
  // throne door + throne room floor
  CHECK_FALSE(m->isBlocked(28, 13));
  CHECK_FALSE(m->isBlocked(28, 11));
  // the walls really are walls (a breach must go through a gate)
  CHECK(m->isBlocked(28, 34));
  CHECK(m->isBlocked(13, 20));
  CHECK(m->isBlocked(43, 20));
}

TEST_CASE("T-123: the moor road leaves the fields by the gnoll camp") {
  // the fields side of the pair: east-rim opening at (63,10)-(63,11)
  server::World w;
  std::string err;
  REQUIRE(w.loadZone(2, "assets/maps/fields_overflow.bhmap", &err));
  const sim::Map* m = w.zoneMap(2);
  REQUIRE(m != nullptr);
  CHECK_FALSE(m->isBlocked(63, 10));
  CHECK_FALSE(m->isBlocked(63, 11));
  CHECK(m->isBlocked(63, 12));  // the rim stays shut between the mouths
}
