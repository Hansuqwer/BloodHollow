// T-109: login/registration rate limiting policy (review §3.5). Pre-fix the
// Hello path had no attempt limits at all: unlimited auto-registered accounts
// (DB-spam DoS) and unlimited password guessing. LoginLimiter is pure
// wall-clock policy — these tests pin every knob the shell relies on.
#include <doctest/doctest.h>

#include <cstdint>

#include "loginlimit.h"

using namespace bh;
using server::LoginLimiter;

namespace {
constexpr std::uint32_t kIpA = 0x01020304;
constexpr std::uint32_t kIpB = 0x05060708;
constexpr std::int64_t kT0 = 1'000'000;  // any positive monotonic ms
}  // namespace

TEST_CASE("T-109: login budget is per-window per-IP and rolls over") {
  LoginLimiter L;
  for (int i = 0; i < LoginLimiter::kMaxLoginsPerWindow; ++i)
    CHECK(L.allowLogin(kIpA, kT0 + i));
  CHECK_FALSE(L.allowLogin(kIpA, kT0 + 100));  // 61st inside the window
  CHECK_FALSE(L.lockedOut(kIpA, kT0 + 100));   // throttled != locked out
  // window rolls after kWindowMs
  CHECK(L.allowLogin(kIpA, kT0 + LoginLimiter::kWindowMs + 1));
  // a different IP is untouched
  CHECK(L.allowLogin(kIpB, kT0 + 101));
}

TEST_CASE("T-109: registration budget is tighter and separate from logins") {
  LoginLimiter L;
  for (int i = 0; i < LoginLimiter::kMaxRegistersPerWindow; ++i)
    CHECK(L.allowRegister(kIpA, kT0 + i));
  CHECK_FALSE(L.allowRegister(kIpA, kT0 + 50));
  // logins keep their own budget (registrations consumed none of it beyond
  // what allowLogin itself would)
  CHECK(L.allowLogin(kIpA, kT0 + 51));
}

TEST_CASE("T-109: 10 consecutive bad credentials lock the IP for 60s") {
  LoginLimiter L;
  for (int i = 0; i < LoginLimiter::kMaxConsecutiveFails - 1; ++i) {
    L.noteFailure(kIpA, kT0 + i);
    CHECK_FALSE(L.lockedOut(kIpA, kT0 + i));
  }
  L.noteFailure(kIpA, kT0 + 9);  // 10th consecutive
  const std::int64_t expiry = kT0 + 9 + LoginLimiter::kLockoutMs;
  CHECK(L.lockedOut(kIpA, kT0 + 9));
  CHECK(L.lockedOut(kIpA, expiry - 1));   // still locked
  CHECK_FALSE(L.lockedOut(kIpA, expiry)); // expired
  CHECK(L.allowLogin(kIpA, expiry + 1));  // usable again
  // other IPs are unaffected by the lockout
  CHECK_FALSE(L.lockedOut(kIpB, kT0 + 10));
}

TEST_CASE("T-109: a success resets the consecutive-failure streak") {
  LoginLimiter L;
  for (int i = 0; i < 9; ++i) L.noteFailure(kIpA, kT0 + i);
  L.noteSuccess(kIpA, kT0 + 20);
  for (int i = 0; i < 9; ++i) L.noteFailure(kIpA, kT0 + 30 + i);
  CHECK_FALSE(L.lockedOut(kIpA, kT0 + 40));  // 9 + 9, never 10 in a row
  L.noteFailure(kIpA, kT0 + 50);             // 10th since the success
  CHECK(L.lockedOut(kIpA, kT0 + 50));
}

TEST_CASE("T-109: prune forgets stale IPs but respects live lockouts") {
  LoginLimiter L;
  L.allowLogin(kIpA, kT0);
  L.noteFailure(kIpB, kT0);
  CHECK(L.trackedIps() == 2);
  L.prune(kT0 + LoginLimiter::kWindowMs + 1);  // both windows rolled, no lockouts
  CHECK(L.trackedIps() == 0);
  // a locked IP survives pruning until the lockout expires
  for (int i = 0; i < LoginLimiter::kMaxConsecutiveFails; ++i)
    L.noteFailure(kIpA, kT0 + i);
  CHECK(L.lockedOut(kIpA, kT0 + 10));
  L.prune(kT0 + LoginLimiter::kWindowMs + 1);
  CHECK(L.trackedIps() == 1);  // lockout still armed
  L.prune(kT0 + LoginLimiter::kLockoutMs + 100'000);
  CHECK(L.trackedIps() == 0);
}
