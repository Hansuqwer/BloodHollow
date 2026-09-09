#include <cstdio>
#include <filesystem>

#include <doctest/doctest.h>

#include "sim/bhmap.h"

using namespace bh;

namespace {

sim::Map makeSample() {
  sim::Map m;
  m.w = 9;
  m.h = 7;
  m.tileW = 64;
  m.tileH = 32;
  m.ground.assign(63, 0);
  m.zone.assign(63, 0);
  m.blocked.assign(63, 0);
  for (int x = 0; x < 9; ++x) {
    m.ground[static_cast<size_t>(x)] = static_cast<std::uint16_t>(x);
    m.blocked[static_cast<size_t>(x)] = 1;  // top border
  }
  m.zone[static_cast<size_t>(4 * 9 + 4)] = 5;
  sim::SpawnDef s;
  s.x = 2;
  s.y = 3;
  s.w = 4;
  s.h = 2;
  s.mobId = 1001;
  s.maxAlive = 6;
  s.respawnTicks = 900;
  s.nightOnly = 1;  // T-071 v2 tail byte
  m.spawners.push_back(s);
  sim::PortalDef p;
  p.x = 8;
  p.y = 6;
  p.w = 1;
  p.h = 1;
  p.targetMapId = 2;
  p.targetX = 3;
  p.targetY = 4;
  m.portals.push_back(p);
  return m;
}

std::string tmpPath(const char* name) {
  return (std::filesystem::temp_directory_path() / name).string();
}

}  // namespace

TEST_CASE("bhmap: write/load round-trips every field") {
  const std::string path = tmpPath("bh_roundtrip_test.bhmap");
  const sim::Map src = makeSample();
  std::string err;
  REQUIRE(sim::saveBhmap(path, src, &err));
  const auto loaded = sim::loadBhmap(path, &err);
  REQUIRE(loaded.has_value());
  CHECK(loaded->w == 9);
  CHECK(loaded->h == 7);
  CHECK(loaded->tileW == 64);
  CHECK(loaded->tileH == 32);
  CHECK(loaded->ground == src.ground);
  CHECK(loaded->zone == src.zone);
  CHECK(loaded->blocked == src.blocked);
  REQUIRE(loaded->spawners.size() == 1);
  CHECK(loaded->spawners[0].mobId == 1001);
  CHECK(loaded->spawners[0].maxAlive == 6);
  CHECK(loaded->spawners[0].respawnTicks == 900);
  CHECK(loaded->spawners[0].nightOnly == 1);
  REQUIRE(loaded->portals.size() == 1);
  CHECK(loaded->portals[0].targetMapId == 2);
  CHECK(loaded->portals[0].targetX == 3);
  CHECK(loaded->portals[0].targetY == 4);
  std::filesystem::remove(path);
}

TEST_CASE("bhmap: checksum catches a corrupted payload") {
  const std::string path = tmpPath("bh_corrupt_test.bhmap");
  const sim::Map src = makeSample();
  std::string err;
  REQUIRE(sim::saveBhmap(path, src, &err));
  {
    FILE* f = std::fopen(path.c_str(), "r+b");
    REQUIRE(f != nullptr);
    std::fseek(f, 0, SEEK_END);
    const long sz = std::ftell(f);
    REQUIRE(sz > 45);
    std::fseek(f, 45, SEEK_SET);  // payload starts at byte 40
    const int c = std::fgetc(f);
    std::fseek(f, 45, SEEK_SET);
    std::fputc(c ^ 0xFF, f);
    std::fclose(f);
  }
  const auto loaded = sim::loadBhmap(path, &err);
  CHECK_FALSE(loaded.has_value());
  CHECK_FALSE(err.empty());
  std::filesystem::remove(path);
}

TEST_CASE("bhmap: loading a missing file reports a clean error") {
  std::string err;
  const auto loaded = sim::loadBhmap(tmpPath("bh_does_not_exist.bhmap"), &err);
  CHECK_FALSE(loaded.has_value());
  CHECK_FALSE(err.empty());
}
