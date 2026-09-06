#pragma once

#include <cstdint>

namespace bh::sim {

// xoshiro256** with splitmix64 seeding.
// Deterministic across Linux/macOS and compilers; never uses <random>
// distributions (those are implementation-defined and would break replay).
// All gameplay randomness flows through this class (ADR-005).
class Rng {
 public:
  explicit Rng(std::uint64_t seed);

  std::uint64_t next();
  // Uniform integer in [lo, hi] (inclusive). Unbiased on targets with
  // 128-bit multiply (all ours); modulo fallback otherwise (negligible bias).
  std::int64_t range(std::int64_t lo, std::int64_t hi);
  // Uniform double in [0, 1).
  double unit();
  // True with probability p.
  bool chance(double p);
  // Uniform float in [lo, hi).
  float frange(float lo, float hi);

  // T-049x: fingerprint of the raw state for BH_DUMP_ENTS replay probes.
  // Pure observation — never advances the stream.
  std::uint64_t stateFingerprint() const {
    std::uint64_t h = 1469598103934665603ULL;
    for (int w = 0; w < 4; ++w) {
      for (int b = 0; b < 8; ++b) {
        h ^= static_cast<std::uint8_t>(s_[w] >> (8 * b));
        h *= 1099511628211ULL;
      }
    }
    return h;
  }

 private:
  static std::uint64_t rotl(std::uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
  std::uint64_t s_[4];
};

}  // namespace bh::sim
