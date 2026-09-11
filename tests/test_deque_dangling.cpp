// T-106: killMob() despawns mid-deque — every read of an Entity reference
// taken BEFORE the kill is dangling AFTER it ([deque.modifiers]: erase
// invalidates all references and iterators). Three call sites read the
// attacker through a stale reference when building the kill chat line:
//   - trySwing primary kill:   att.kind after killMob(def, ...)
//   - trySwing cleave sweep:   killer->kind / killer->name after killMob(*cv, ...)
//   - trySkill power swing:    e.name after killMob(*target, ...)
// libstdc++ deque erase relocates the nearer end's elements, so the stale
// slot usually holds a shifted neighbour (or a moved-from husk): the kill
// line is then suppressed (kind read as kMob) or credits the WRONG entity
// (name read from the neighbour). These tests pin the deque layout so the
// pre-fix code observably mis-reports and the snapshot-fix reports truth.
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

#include "content/mobs.h"
#include "world.h"

using namespace bh;

namespace {
sim::Map makeArena() {
  sim::Map m;
  m.w = 40;
  m.h = 40;
  m.tileW = 64;
  m.tileH = 32;
  m.ground.assign(40 * 40, 0);
  m.zone.assign(40 * 40, 0);
  m.blocked.assign(40 * 40, 0);
  return m;
}

server::Entity* spawnP(server::World& w, const char* name, int x, int y) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y});
  return w.find(e.id);
}

// first kind==3 event aimed at `target`; returns false if none seen yet
bool killLine(const server::World& w, std::uint32_t target, std::string& out) {
  for (const auto& ev : w.events()) {
    if (ev.kind == 3 && ev.target == target) {
      out = ev.chatText;
      return true;
    }
  }
  return false;
}

void buff(server::Entity& p) {
  p.hp = 400;
  p.hpMax = 400;
  p.dex = 60;  // acc = 2*dex: misses become rare
  p.str = 60;
}
}  // namespace

TEST_CASE("T-106: plain-swing kill credits the true killer (att read post-erase)") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const content::MobDef& rat = *content::findMob(1001);
  const content::MobDef& ghoul = *content::findMob(1002);
  REQUIRE(rat.name != ghoul.name);
  // deque order: [loadFrom NPCs..., victim, player, tail]. Erasing the victim
  // sits in the rear half -> libstdc++ shifts the rear left -> the player's
  // old slot holds the tail mob, so a post-kill `att.kind` read sees kMob.
  server::Entity& m1 = w.debugSpawnMob(rat, sim::TilePos{10, 10});
  server::Entity* p = spawnP(w, "Ada", 11, 10);
  server::Entity& tail = w.debugSpawnMob(ghoul, sim::TilePos{25, 25});
  m1.hp = 1;
  m1.dex = 0;
  tail.hp = 5000;
  tail.hpMax = 5000;
  buff(*p);
  p->attackTarget = m1.id;
  const std::uint32_t m1Id = m1.id;
  const std::string expect = std::string("Ada has slain a ") + m1.name + ".";
  std::string got;
  bool saw = false;
  for (int i = 0; i < 600 && !saw; ++i) {
    w.tick();
    saw = killLine(w, m1Id, got);
  }
  REQUIRE(saw);
  CHECK(got == expect);  // pre-fix: "" (line suppressed — slot read as mob)
}

TEST_CASE("T-106: cleave sweep credits the true killer for deferred kills") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const content::MobDef& rat = *content::findMob(1001);
  // deque order: [NPCs..., player, primary, cleave-victim, 13 far fillers].
  // Both erases land in the FRONT half -> libstdc++ shifts the front right ->
  // the attacker's and the killer's old slots are overwritten by neighbours,
  // so post-killMob kind/name reads see the wrong entity on BOTH kill lines.
  server::Entity* p = spawnP(w, "Bo", 11, 10);
  server::Entity& m1 = w.debugSpawnMob(rat, sim::TilePos{10, 10});
  server::Entity& m2 = w.debugSpawnMob(rat, sim::TilePos{11, 11});  // chebyshev 1 from m1
  for (int i = 0; i < 13; ++i) {
    server::Entity& f =
        w.debugSpawnMob(rat, sim::TilePos{30 + (i % 4), 30 + (i / 4)});
    f.hp = 5000;
    f.hpMax = 5000;
  }
  REQUIRE(w.debugGive(*p, 2001, 1));  // Rusty Shank (weapon, slot 0)
  int shankSlot = -1;
  for (size_t i = 0; i < p->inv.size(); ++i)
    if (p->inv[i].itemId == 2001) shankSlot = static_cast<int>(i);
  REQUIRE(shankSlot >= 0);
  REQUIRE(w.toggleEquip(*p, static_cast<std::uint8_t>(shankSlot)));
  bool auraSet = false;
  for (auto& sl : p->inv) {
    if (!sl.equipped) continue;
    sl.aura = 3;  // tier III WIDOW'S EDGE: cleave, 100% hit, no rng
    sl.durability = 100;
    auraSet = true;
  }
  REQUIRE(auraSet);
  m1.hp = 1;
  m2.hp = 1;
  m1.dex = 0;
  m2.dex = 0;
  buff(*p);
  p->attackTarget = m1.id;
  const std::uint32_t m1Id = m1.id;
  const std::uint32_t m2Id = m2.id;
  const std::string expect1 = std::string("Bo has slain a ") + m1.name + ".";
  const std::string expect2 = std::string("Bo has slain a ") + m2.name + ".";
  std::string got1, got2;
  bool saw1 = false, saw2 = false;
  for (int i = 0; i < 600 && !(saw1 && saw2); ++i) {
    w.tick();
    if (!saw1) saw1 = killLine(w, m1Id, got1);
    if (!saw2) saw2 = killLine(w, m2Id, got2);
  }
  REQUIRE(saw1);
  REQUIRE(saw2);  // sweep executed the deferred cleave kill
  CHECK(got1 == expect1);  // pre-fix: "" (primary, front-shift overwrote att)
  CHECK(got2 == expect2);  // pre-fix: "" (sweep, killer slot overwritten)
}

TEST_CASE("T-106: power-swing kill credits the true killer's NAME") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  const content::MobDef& rat = *content::findMob(1001);
  const content::MobDef& ghoul = *content::findMob(1002);
  // Same rear-half layout as case 1: after the victim's erase the player's
  // old slot holds the ghoul, so the post-killMob `e.name` read credits the
  // ghoul: "<ghoul> has slain a <rat>." instead of "Cy has slain a <rat>."
  server::Entity& m1 = w.debugSpawnMob(rat, sim::TilePos{10, 10});
  server::Entity* p = spawnP(w, "Cy", 11, 10);
  server::Entity& tail = w.debugSpawnMob(ghoul, sim::TilePos{25, 25});
  m1.hp = 1;
  m1.dex = 0;
  tail.hp = 5000;
  tail.hpMax = 5000;
  buff(*p);
  p->level = 99;  // clear any kit level gate on skill 1
  const std::uint32_t pId = p->id;
  const std::uint32_t m1Id = m1.id;
  const std::string expect = std::string("Cy has slain a ") + m1.name + ".";
  w.tick();  // drain boot events
  std::string got;
  bool saw = false;
  for (int attempt = 0; attempt < 30 && !saw; ++attempt) {
    server::Entity* pp = w.find(pId);
    server::Entity* mm = w.find(m1Id);
    REQUIRE(pp != nullptr);
    if (mm == nullptr) break;  // died some other way — fail below with saw=false
    const sim::TilePos mt = mm->walker.tile();
    pp->walker.place(sim::TilePos{mt.x + 1, mt.y});  // re-glue (mobs wander)
    w.trySkill(*pp, 1, m1Id);                        // power swing (skill 1)
    saw = killLine(w, m1Id, got);
    if (!saw) {
      w.tick();
      for (int k = 0; k < 41 && !saw; ++k) {  // kPowerSwingCdTicks = 40
        w.tick();
        saw = killLine(w, m1Id, got);  // no attackTarget set: only the skill can kill
      }
    }
  }
  REQUIRE(saw);
  CHECK(got == expect);
}
