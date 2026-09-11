#pragma once
// T-109 (review §3.5): login/registration rate limiting.
// Pre-fix, the Hello path had NO attempt limits: any peer could spin
// unlimited auto-registered accounts (DB-spam DoS) and unlimited password
// guesses against the (stub, ADR-0009) hash. This is a pure wall-clock-policy
// helper — no ENet, no SQLite — so bh_tests can pin the policy directly.
//
// Policy (prototype posture; bot legs create <= 8 accounts and never send
// bad credentials, so legit harnesses are unaffected by orders of magnitude):
//   - logins:       <= 60 attempts / 60 s window / source IP
//   - registrations: <= 30 new accounts / 60 s window / source IP
//   - credentials:  10 consecutive bad-password failures / IP -> 60 s lockout
//                   (any success resets the consecutive counter)
// Callers pass a monotonic millisecond clock (steady_clock); the limiter
// never reads a clock itself, which keeps it deterministic under test.
#include <cstdint>
#include <cstddef>
#include <unordered_map>

namespace bh::server {

class LoginLimiter {
 public:
  static constexpr int kMaxLoginsPerWindow = 60;
  static constexpr int kMaxRegistersPerWindow = 30;
  static constexpr std::int64_t kWindowMs = 60 * 1000;
  static constexpr int kMaxConsecutiveFails = 10;
  static constexpr std::int64_t kLockoutMs = 60 * 1000;

  // True while `ip` is inside a bad-credentials lockout.
  bool lockedOut(std::uint32_t ip, std::int64_t nowMs) const {
    const auto it = perIp_.find(ip);
    return it != perIp_.end() && it->second.lockoutUntilMs > nowMs;
  }

  // Consumes one login attempt; false when the window budget is exhausted.
  bool allowLogin(std::uint32_t ip, std::int64_t nowMs) {
    State& st = touch(ip, nowMs);
    if (st.logins >= kMaxLoginsPerWindow) return false;
    ++st.logins;
    return true;
  }

  // Consumes one registration; false when the (tighter) window budget is gone.
  bool allowRegister(std::uint32_t ip, std::int64_t nowMs) {
    State& st = touch(ip, nowMs);
    if (st.registers >= kMaxRegistersPerWindow) return false;
    ++st.registers;
    return true;
  }

  // Bad credentials for `ip`; the 10th consecutive failure arms the lockout.
  void noteFailure(std::uint32_t ip, std::int64_t nowMs) {
    State& st = touch(ip, nowMs);
    ++st.fails;
    if (st.fails >= kMaxConsecutiveFails) {
      st.lockoutUntilMs = nowMs + kLockoutMs;
      st.fails = 0;
    }
  }

  // A successful login clears the consecutive-failure streak.
  void noteSuccess(std::uint32_t ip, std::int64_t nowMs) { touch(ip, nowMs).fails = 0; }

  // Drop entries whose window has rolled and whose lockout (if any) expired;
  // call on disconnects / periodically so the map tracks live peers only.
  void prune(std::int64_t nowMs) {
    for (auto it = perIp_.begin(); it != perIp_.end();) {
      const State& st = it->second;
      const bool staleWindow =
          st.windowValid && nowMs - st.windowStartMs >= kWindowMs;
      const bool noWindow = !st.windowValid;
      if ((staleWindow || noWindow) && st.lockoutUntilMs <= nowMs)
        it = perIp_.erase(it);
      else
        ++it;
    }
  }

  std::size_t trackedIps() const { return perIp_.size(); }

 private:
  struct State {
    bool windowValid = false;
    std::int64_t windowStartMs = 0;
    int logins = 0;
    int registers = 0;
    int fails = 0;  // consecutive bad credentials
    std::int64_t lockoutUntilMs = 0;
  };

  State& touch(std::uint32_t ip, std::int64_t nowMs) {
    State& st = perIp_[ip];
    if (!st.windowValid || nowMs - st.windowStartMs >= kWindowMs) {
      st.windowValid = true;
      st.windowStartMs = nowMs;
      st.logins = 0;
      st.registers = 0;
    }
    return st;
  }

  std::unordered_map<std::uint32_t, State> perIp_{};
};

}  // namespace bh::server
