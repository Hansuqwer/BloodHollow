// T-067 procedural audio — synthesized once at boot into raylib Sound
// buffers. No assets, no network; the whole kit is trigonometry and a
// local LCG. Budget: ~78 KB of 22050 Hz / 16-bit / mono samples.
//
// Signed convention per callout palette:
//   swing  — 0.10s sine swipe 220→70 Hz (blade leaving the sheath)
//   hit    — 0.06s noise thud, lowpass-biased (meat)
//   bolt   — 0.22s downward chirp + breath noise (the Gravemother's air)
//   toll   — 0.45s FM-ish metallic ping, long decay (anvil ceremony)
//   choir  — 0.35s two-note minor-third with soft onset (mend/bless sting)
//   chime  — 0.28s two-tone rise (level-up)
//   death  — 0.30s low thud + slow downward slide (petrify-fade partner)
#pragma once

#include <cmath>
#include <cstdint>

#include <raylib.h>

namespace bh {

struct SynthKit {
  Sound swing{}, hit{}, bolt{}, toll{}, choir{}, chime{}, death{};
  bool ready = false;

  static constexpr double kRate = 22050.0;

  // deterministic 64-bit LCG (the same family the map generators use — the
  // noise is content, not entropy)
  static std::uint64_t lcg(std::uint64_t x) {
    return x * 6364136223846793005ULL + 1442695040888963407ULL;
  }

  static Sound bake(const char* name, double secs,
                    void (*voice)(double t, double norm, std::int16_t* out,
                                  std::uint64_t* lcg)) {
    const int frames = static_cast<int>(secs * kRate);
    auto* pcm = static_cast<std::int16_t*>(MemAlloc(sizeof(std::int16_t) * frames));
    std::uint64_t seed = 0xB100D10CA110LL;
    for (int i = 0; i < frames; ++i) {
      voice(static_cast<double>(i) / kRate, static_cast<double>(i) / frames,
            &pcm[i], &seed);
    }
    Wave w{};
    w.frameCount = static_cast<unsigned int>(frames);
    w.sampleRate = 22050;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = pcm;
    Sound s = LoadSoundFromWave(w);
    MemFree(pcm);
    TraceLog(LOG_INFO, "synthkit: baked %s (%d frames)", name, frames);
    return s;
  }

  static constexpr double kTwoPi = 6.283185307179586;

  static void vSwing(double t, double n, std::int16_t* out, std::uint64_t* l) {
    (void)l;
    const double f = 220.0 + (70.0 - 220.0) * n;  // falling swipe
    const double env = (1.0 - n) * (1.0 - n);
    *out = static_cast<std::int16_t>(std::sin(kTwoPi * f * t) * env * 24000);
  }
  static void vHit(double t, double n, std::int16_t* out, std::uint64_t* l) {
    (void)t;
    *l = lcg(*l);
    const double noise = (static_cast<double>((*l >> 33) & 0xFFFF) / 65535.0) - 0.5;
    const double body = std::sin(kTwoPi * 90.0 * t);
    const double env = (1.0 - n) * (1.0 - n);
    *out = static_cast<std::int16_t>((noise * 0.7 + body * 0.5) * env * 22000);
  }
  static void vBolt(double t, double n, std::int16_t* out, std::uint64_t* l) {
    const double f = 620.0 + (95.0 - 620.0) * n;  // long downward chirp
    *l = lcg(*l);
    const double breath = (static_cast<double>((*l >> 33) & 0xFFFF) / 65535.0) - 0.5;
    const double env = (1.0 - n);
    *out = static_cast<std::int16_t>((std::sin(kTwoPi * f * t) * 0.6 + breath * 0.25) *
                                     env * 21000);
  }
  static void vToll(double t, double n, std::int16_t* out, std::uint64_t* l) {
    (void)l;
    // metallic ping: carrier + inharmonic partials, x^n decay
    const double s = std::sin(kTwoPi * 660.0 * t) +
                     0.6 * std::sin(kTwoPi * 660.0 * 2.76 * t) +
                     0.35 * std::sin(kTwoPi * 660.0 * 5.40 * t);
    const double env = std::pow(1.0 - n, 3.0);
    *out = static_cast<std::int16_t>(s * env * 9000);
  }
  static void vChoir(double t, double n, std::int16_t* out, std::uint64_t* l) {
    (void)l; (void)t;
    const double root = n < 0.5 ? 392.0 : 466.16;  // G4 -> Bb4 minor third
    const double onset = n < 0.12 ? n / 0.12 : 1.0;
    *out = static_cast<std::int16_t>(std::sin(kTwoPi * root * (n * 0.35)) *
                                     onset * (1.0 - n) * 0.35 * 16000 +
                                     std::sin(kTwoPi * root * 2.0 * (n * 0.35)) *
                                     onset * (1.0 - n) * 0.15 * 14000);
  }
  static void vChime(double t, double n, std::int16_t* out, std::uint64_t* l) {
    (void)l; (void)t;
    const double f = n < 0.4 ? 523.25 : 783.99;  // C5 then G5
    const double env = n < 0.4 ? 1.0 : (1.0 - (n - 0.4) / 0.6);
    *out = static_cast<std::int16_t>(std::sin(kTwoPi * f * (n * 0.28)) * env * 14000);
  }
  static void vDeath(double t, double n, std::int16_t* out, std::uint64_t* l) {
    *l = lcg(*l);
    const double rumble = std::sin(kTwoPi * (60.0 - 30.0 * n) * t);
    const double dust = (static_cast<double>((*l >> 33) & 0xFFFF) / 65535.0) - 0.5;
    const double env = std::pow(1.0 - n, 2.0);
    *out = static_cast<std::int16_t>((rumble * 0.7 + dust * 0.2) * env * 20000);
  }

  void init() {
    if (ready) return;
    swing = bake("swing", 0.10, &vSwing);
    hit = bake("hit", 0.06, &vHit);
    bolt = bake("bolt", 0.22, &vBolt);
    toll = bake("toll", 0.45, &vToll);
    choir = bake("choir", 0.35, &vChoir);
    chime = bake("chime", 0.28, &vChime);
    death = bake("death", 0.30, &vDeath);
    ready = true;
  }
  void unload() {
    if (!ready) return;
    UnloadSound(swing); UnloadSound(hit); UnloadSound(bolt); UnloadSound(toll);
    UnloadSound(choir); UnloadSound(chime); UnloadSound(death);
    ready = false;
  }
};

}  // namespace bh
