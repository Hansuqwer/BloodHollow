#include <doctest/doctest.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "protocol/messages_gen.h"

using namespace bh::proto;

TEST_CASE("protogen: generated messages roundtrip") {
  SUBCASE("Hello (strings)") {
    Hello in;
    in.protoVersion = kProtocolVersion;
    in.username = "someone";
    in.password = "s3cret-ish";
    const auto bytes = pack(in);
    const auto pv = view(bytes.data(), bytes.size());
    CHECK(pv.ok);
    CHECK(pv.id == Hello::kId);
    Hello out;
    CHECK(out.deserialize(pv.body));
    CHECK(out.protoVersion == in.protoVersion);
    CHECK(out.username == "someone");
    CHECK(out.password == "s3cret-ish");
  }
  SUBCASE("Welcome (many scalars)") {
    Welcome in;
    in.entityId = 42;
    in.mapId = 7;
    in.x = -12345;
    in.y = 65000;
    in.tick = 987654321u;
    in.hourCenti = 2175;
    const auto bytes = pack(in);
    const auto pv = view(bytes.data(), bytes.size());
    REQUIRE(pv.ok);
    Welcome out;
    REQUIRE(out.deserialize(pv.body));
    CHECK(out.entityId == 42u);
    CHECK(out.x == -12345);
    CHECK(out.y == 65000);
    CHECK(out.tick == 987654321u);
    CHECK(out.hourCenti == 2175u);
  }
  SUBCASE("EntityDelta (Q10 + flags)") {
    EntityDelta in;
    in.id = 99;
    in.x = 23456;
    in.y = -789;
    in.dir = 5;
    in.moving = 1;
    in.hp = 30;
    in.glowTier = 1;  // T-092 trailing field round-trips
    const auto bytes = pack(in);
    const auto pv = view(bytes.data(), bytes.size());
    REQUIRE(pv.ok);
    EntityDelta out;
    REQUIRE(out.deserialize(pv.body));
    CHECK(out.id == 99u);
    CHECK(out.x == 23456);
    CHECK(out.y == -789);
    CHECK(out.moving == 1);
    CHECK(out.hp == 30u);
    CHECK(out.glowTier == 1u);
  }
  SUBCASE("EntitySpawn carries glowTier (T-092)") {
    EntitySpawn in;
    in.id = 7;
    in.kind = 0;
    in.name = "glowy";
    in.glowTier = 2;
    const auto bytes = pack(in);
    const auto pv = view(bytes.data(), bytes.size());
    REQUIRE(pv.ok);
    EntitySpawn out;
    REQUIRE(out.deserialize(pv.body));
    CHECK(out.id == 7u);
    CHECK(out.name == "glowy");
    CHECK(out.glowTier == 2u);
    CHECK(out.light == 0u);  // neighbours default, not clobbered
  }
}

TEST_CASE("protogen: every truncated prefix fails to deserialize") {
  // one fixed-size message
  {
    InputStep in;
    in.dx = -1;
    in.dy = 1;
    const auto full = pack(in);
    for (size_t cut = 0; cut + 1 < full.size(); ++cut) {
      const auto pv = view(full.data(), cut);
      if (pv.ok) {  // view ok only if prefix still holds a full frame
        InputStep out;
        CHECK_FALSE(out.deserialize(pv.body));
      }
    }
    // also: full header but truncated payload region (craft frame)
    std::vector<std::uint8_t> crafted = full;
    crafted.resize(crafted.size() - 1);
    crafted[2] = static_cast<std::uint8_t>(crafted.size() - 4);
    crafted[3] = 0;
    const auto pv = view(crafted.data(), crafted.size());
    REQUIRE(pv.ok);  // frame itself is valid, body is 1 byte short
    InputStep out;
    CHECK_FALSE(out.deserialize(pv.body));
  }
  // one string-bearing message
  {
    Hello in;
    in.protoVersion = 1;
    in.username = "alice_room";
    in.password = "pw";
    const auto full = pack(in);
    for (size_t cut = 0; cut + 1 < full.size(); ++cut) {
      const auto pv = view(full.data(), cut);
      if (pv.ok) {
        Hello out;
        CHECK_FALSE(out.deserialize(pv.body));
      }
    }
  }
}

TEST_CASE("protogen: unknown ids and framing edge cases") {
  CHECK(std::strcmp(msgName(kIdHello), "Hello") == 0);
  CHECK(std::strcmp(msgName(0), "unknown") == 0);
  CHECK(std::strcmp(msgName(9999), "unknown") == 0);

  // view() rejects: null, short, length mismatch, trailing junk
  CHECK_FALSE(view(nullptr, 0).ok);
  const std::uint8_t tiny[3] = {1, 0, 0};
  CHECK_FALSE(view(tiny, 3).ok);
  Ping p;
  p.clientTimeMs = 5;
  auto bytes = pack(p);
  bytes.push_back(0xEE);  // trailing junk beyond declared len
  CHECK_FALSE(view(bytes.data(), bytes.size()).ok);
  // corrupted inner length
  auto bad = pack(p);
  bad[3]++;  // declared len no longer matches actual
  CHECK_FALSE(view(bad.data(), bad.size()).ok);
}
