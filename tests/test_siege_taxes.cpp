// T-134 siege taxes + vault + holder buff + persist (Phase S, 3/4).
// Pins: tithe math (twin worlds), buff on/off, Db round-trip + empty,
// boot-load + readout, crown booking, save throttle.
#include <doctest/doctest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

#include "content/mobs.h"
#include "content/wirekind.h"
#include "command.h"
#include "persist.h"
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

server::Entity* spawnP(server::World& w, const char* name, int x, int y,
                       std::uint16_t zone = 1) {
  server::Entity& e = w.spawn(name, 0, sim::TilePos{x, y}, zone);
  return w.find(e.id);
}

std::string tmpDbPath(const char* name) {
  return (std::filesystem::temp_directory_path() / name).string();
}

void rmDb(const std::string& p) {
  std::filesystem::remove(p);
  std::filesystem::remove(p + "-wal");
  std::filesystem::remove(p + "-shm");
}
}  // namespace

TEST_CASE("T-134: castle tax diverts 5% while a holder stands") {
  // Twin worlds, hound prey (gold 26..48 => tithe always >= 1); the ONLY
  // difference is the loaded holder. Same stream => exact tithe math.
  auto run = [](bool held) {
    server::World w;
    w.loadFrom(makeArena());
    if (held) w.loadSiegeState(7, "Ashen", 0, 1);
    server::Entity* p = spawnP(w, held ? "taxed" : "free", 5, 5);
    p->hpMax = 2000;
    p->hp = 2000;
    w.debugGive(*p, 2002, 1);  // Pit Blade
    for (std::uint8_t i = 0; i < p->inv.size(); ++i)
      if (p->inv[i].itemId == 2002) {
        w.toggleEquip(*p, i);
        break;
      }
    server::Entity* hound =
        w.find(w.debugSpawnMob(*content::findMob(1003), {6, 5}).id);
    const std::uint32_t hid = hound->id;
    const std::uint32_t pid = p->id;
    const std::uint32_t gold0 = p->gold;
    server::Command c;
    c.kind = server::Command::kAttack;
    c.a = static_cast<std::int32_t>(hid);
    server::applyWorldCommand(w, *w.find(pid), c);
    int guard = 600;
    while (w.find(hid) != nullptr && guard-- > 0) w.tick();
    REQUIRE(w.find(hid) == nullptr);
    return std::pair<std::uint32_t, std::uint32_t>(
        w.find(pid)->gold - gold0, w.siegeVault());
  };
  const auto [freeGold, freeVault] = run(false);
  const auto [taxedGold, vault] = run(true);
  CHECK(freeVault == 0u);  // no holder: no tithe
  REQUIRE(vault >= 1u);  // hound floor guarantees a non-trivial tithe
  CHECK(vault == freeGold * 5u / 100u);
  CHECK(taxedGold == freeGold - vault);
}

TEST_CASE("T-134: the holder acts blessed, the unseated do not") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  server::Entity* p = spawnP(w, "claimant", 5, 5);
  w.debugGive(*p, 2001, 1);
  for (std::uint8_t i = 0; i < p->inv.size(); ++i)
    if (p->inv[i].itemId == 2001) {
      w.toggleEquip(*p, i);
      break;
    }
  const std::uint32_t acc0 = w.effAcc(*p);      // 2*8 = 16
  const std::uint32_t dmg0 = w.effDmgBase(*p);  // shank 12
  w.loadSiegeState(p->id, "claimant", 0, 1);
  CHECK(w.effAcc(*p) == acc0 * 110u / 100u);
  CHECK(w.effDmgBase(*p) == dmg0 * 110u / 100u);
  w.loadSiegeState(0, "", 0, 1);  // unseated
  CHECK(w.effAcc(*p) == acc0);
  CHECK(w.effDmgBase(*p) == dmg0);
}

TEST_CASE("T-134: siege_state round-trips, empty loads zero") {
  const std::string path = tmpDbPath("bh_siege_roundtrip.bhdb");
  rmDb(path);
  server::Db db;
  std::string err;
  REQUIRE(db.open(path, &err));
  server::Db::SiegeRow empty{};
  REQUIRE(db.loadSiege(&empty, &err));
  CHECK(empty.holderId == 0);
  CHECK(empty.holderName == "");
  CHECK(empty.vaultGold == 0);
  CHECK(empty.crowns == 0);
  REQUIRE(db.saveSiege(7, "Ashen", 1234, 2, &err));
  server::Db::SiegeRow row{};
  REQUIRE(db.loadSiege(&row, &err));
  CHECK(row.holderId == 7);
  CHECK(row.holderName == "Ashen");
  CHECK(row.vaultGold == 1234);
  CHECK(row.crowns == 2);
  REQUIRE(db.saveSiege(9, "Synod", 500, 3, &err));  // overwrite
  REQUIRE(db.loadSiege(&row, &err));
  CHECK(row.holderId == 9);
  CHECK(row.vaultGold == 500);
  CHECK(row.crowns == 3);
  rmDb(path);
}

TEST_CASE("T-134: boot load + gm siege readout") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  w.loadSiegeState(7, "Ashen", 500, 2);
  CHECK(w.siegeHolder() == 7u);
  CHECK(w.siegeHolderName() == "Ashen");
  CHECK(w.siegeVault() == 500u);
  CHECK(w.siegeCrowns() == 2u);
  CHECK_FALSE(w.siegeDirty());  // loading is not accruing
  server::Entity* p = spawnP(w, "reader", 5, 5);
  w.siegeReadout(*p);
  bool castle = false, vault = false, bands = false;
  for (const auto& ev : w.events()) {
    if (ev.chatCh != 255 || ev.aboutId != p->id) continue;
    if (ev.chatText.find("Ashen") != std::string::npos) castle = true;
    if (ev.chatText.find("500g") != std::string::npos) vault = true;
    if (ev.chatText.find("bands:") != std::string::npos) bands = true;
  }
  CHECK(castle);
  CHECK(vault);
  CHECK(bands);
}

TEST_CASE("T-134: crowning books holder, crowns, and dirt") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  REQUIRE(w.loadZoneFrom(6, makeArena()));
  server::Entity* a = spawnP(w, "monarch", 5, 5, 6);
  REQUIRE(w.siegeRegister(a->id));
  w.debugSetTick(1872000);
  REQUIRE(w.siegeStart(*a));
  for (const auto& e : w.entities()) {
    if (e.wireKind == content::kWireKindHeartstone && e.zoneId == 6) {
      const sim::TilePos g = e.walker.tile();
      a->walker.place(sim::TilePos{g.x + 1, g.y});
      break;
    }
  }
  for (int t = 0; t < 1200; ++t) w.tick();
  REQUIRE(w.heartAttuned());
  REQUIRE(w.crown(*a));
  CHECK_FALSE(w.siegeDirty());
  for (int t = 0; t < 200; ++t) w.tick();
  CHECK(w.siegeHolder() == a->id);
  CHECK(w.siegeHolderName() == "monarch");
  CHECK(w.siegeCrowns() == 1u);
  CHECK(w.siegeDirty());
}

TEST_CASE("T-134: save throttle is 600 ticks") {
  server::World w;
  REQUIRE(w.loadFrom(makeArena()));
  CHECK_FALSE(w.siegeSaveDue(w.tickCount()));  // clean: never due
  w.loadSiegeState(7, "Ashen", 0, 1);
  CHECK_FALSE(w.siegeSaveDue(w.tickCount()));  // loading is not accruing
  // accrue with a rat kill (tithe may be 0 — dirtiness is what matters)
  server::Entity* p = spawnP(w, "taxman", 5, 5);
  w.debugGive(*p, 2002, 1);
  for (std::uint8_t i = 0; i < p->inv.size(); ++i)
    if (p->inv[i].itemId == 2002) {
      w.toggleEquip(*p, i);
      break;
    }
  auto killRat = [&]() {
    server::Entity* rat =
        w.find(w.debugSpawnMob(*content::findMob(1001), {6, 5}).id);
    const std::uint32_t rid = rat->id;
    server::Command c;
    c.kind = server::Command::kAttack;
    c.a = static_cast<std::int32_t>(rid);
    server::applyWorldCommand(w, *w.find(p->id), c);
    int guard = 400;
    while (w.find(rid) != nullptr && guard-- > 0) w.tick();
    REQUIRE(w.find(rid) == nullptr);
  };
  killRat();
  const sim::Tick t0 = w.tickCount();
  REQUIRE(w.siegeDirty());
  CHECK(w.siegeSaveDue(t0));
  w.markSiegeSaved(t0);
  CHECK_FALSE(w.siegeSaveDue(t0 + 599));
  killRat();  // accrue again inside the window: still suppressed
  CHECK_FALSE(w.siegeSaveDue(w.tickCount()));
  w.debugSetTick(t0 + 600);
  CHECK(w.siegeSaveDue(w.tickCount()));  // window elapsed: due again
}
