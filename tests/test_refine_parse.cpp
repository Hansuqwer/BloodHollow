// T-104 regression pins: "/refine <n>" chat-command argument parsing.
// Pre-fix, server/src/main.cpp ran std::stoi(m.text.substr(8)) after an
// all-digits-only check, so ONE authenticated chat packet
// ("/refine 99999999999999") escaped handlePacket as std::out_of_range and
// aborted the entire world server (reproduced: SIGABRT, all sessions dropped).
// The parse now lives in command.h as parseRefineArg and must never throw.
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

#include "command.h"

using bh::server::parseRefineArg;

TEST_CASE("T-104: parseRefineArg accepts in-domain slots") {
  std::int32_t v = -1;
  CHECK(parseRefineArg("0", v));
  CHECK(v == 0);
  CHECK(parseRefineArg("7", v));
  CHECK(v == 7);
  CHECK(parseRefineArg("42", v));
  CHECK(v == 42);
  CHECK(parseRefineArg("255", v));
  CHECK(v == 255);
}

TEST_CASE("T-104: parseRefineArg rejects the crash class (int overflow)") {
  std::int32_t v = 0;
  // The exact payload that aborted the pre-fix server:
  CHECK_FALSE(parseRefineArg("99999999999999", v));
  CHECK_FALSE(parseRefineArg("2147483648", v));     // INT_MAX + 1
  CHECK_FALSE(parseRefineArg("18446744073709551617", v));  // > u64 too
  CHECK_FALSE(parseRefineArg("99999999999999999999999999", v));
}

TEST_CASE("T-104: parseRefineArg rejects out-of-domain and malformed args") {
  std::int32_t v = 0;
  CHECK_FALSE(parseRefineArg("", v));        // empty
  CHECK_FALSE(parseRefineArg("256", v));     // slot domain is u8 on the wire
  CHECK_FALSE(parseRefineArg("-1", v));      // sign is not a digit
  CHECK_FALSE(parseRefineArg("12a", v));     // trailing garbage
  CHECK_FALSE(parseRefineArg("a12", v));     // leading garbage
  CHECK_FALSE(parseRefineArg(" 12", v));     // leading space
  CHECK_FALSE(parseRefineArg("1 2", v));     // inner space
  CHECK_FALSE(parseRefineArg("+12", v));     // explicit plus
  CHECK_FALSE(parseRefineArg("12\n", v));    // newline (pre-sanitize raw text)
  CHECK_FALSE(parseRefineArg("\xC3\xA9", v));  // non-ASCII UTF-8
}

TEST_CASE("T-104: parseRefineArg leaves out untouched on rejection") {
  std::int32_t v = 1234;
  CHECK_FALSE(parseRefineArg("99999999999999", v));
  CHECK(v == 1234);  // no partial write
}
