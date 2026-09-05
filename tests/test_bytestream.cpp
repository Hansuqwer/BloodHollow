#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "protocol/bytestream.h"

using bh::proto::Reader;
using bh::proto::Writer;

TEST_CASE("bytestream roundtrips every scalar type") {
  std::vector<std::uint8_t> buf;
  Writer w(buf);
  w.u8(0xAB);
  w.u16(0xBEEF);
  w.u32(0xDEADBEEFu);
  w.u64(0x0123456789ABCDEFull);
  w.i8(-7);
  w.i16(-1234);
  w.i32(-123456);
  w.i64(-1234567890123ll);
  w.f32(-13.75f);
  w.boolean(true);
  w.boolean(false);
  w.str("bloodhollow");
  w.str("");

  Reader r(buf.data(), buf.size());
  std::uint8_t a;
  std::uint16_t b;
  std::uint32_t c;
  std::uint64_t d;
  std::int8_t e;
  std::int16_t f;
  std::int32_t g;
  std::int64_t h;
  float i;
  bool j, k;
  std::string s1, s2;
  CHECK(r.u8(a));
  CHECK(r.u16(b));
  CHECK(r.u32(c));
  CHECK(r.u64(d));
  CHECK(r.i8(e));
  CHECK(r.i16(f));
  CHECK(r.i32(g));
  CHECK(r.i64(h));
  CHECK(r.f32(i));
  CHECK(r.boolean(j));
  CHECK(r.boolean(k));
  CHECK(r.str(s1));
  CHECK(r.str(s2));
  CHECK(std::fabs(i - -13.75f) < 1e-6f);
  CHECK(s1 == "bloodhollow");
  CHECK(s2.empty());
  CHECK(r.remaining() == 0);

  // LE sanity: first u32 written must be little-endian.
  CHECK(buf[3] == 0xEF);
  CHECK(buf[4] == 0xBE);
}

TEST_CASE("bytestream reader is strictly bounded") {
  std::vector<std::uint8_t> buf;
  Writer w(buf);
  w.u32(0x11223344u);
  // every truncation of the 4-byte u32 must fail
  for (size_t cut = 0; cut < buf.size(); ++cut) {
    Reader r(buf.data(), cut);
    std::uint32_t v = 0;
    CHECK_FALSE(r.u32(v));
  }
}

TEST_CASE("bytestream str is length-capped and truncation-safe") {
  std::vector<std::uint8_t> buf;
  Writer w(buf);
  std::string longs(5000, 'x');
  w.str(longs);
  for (size_t cut = 0; cut < buf.size(); ++cut) {
    Reader r(buf.data(), cut);
    std::string v;
    CHECK_FALSE(r.str(v, 4096));
  }
  // a valid encoding over the 4096 cap fails too (attacker sends len > cap)
  std::vector<std::uint8_t> bad;
  Writer bw(bad);
  bw.u16(4097);
  Reader br(bad.data(), bad.size());
  std::string v;
  CHECK_FALSE(br.str(v, 4096));

  // writer hard-caps at 65535
  std::vector<std::uint8_t> big;
  Writer bigw(big);
  bigw.str(std::string(70000, 'y'));
  Reader bigr(big.data(), big.size());
  std::string got;
  CHECK(bigr.str(got, 70000));
  CHECK(got.size() == 65535);
}
