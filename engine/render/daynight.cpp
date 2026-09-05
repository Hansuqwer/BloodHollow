#include "render/daynight.h"

namespace bh {

Color nightOverlay(float hour) {
  struct Key {
    float h;
    unsigned char r, g, b, a;
  };
  static const Key kKeys[] = {
      {0.0f, 18, 22, 70, 140}, {4.0f, 18, 22, 70, 150}, {6.0f, 80, 60, 60, 60},
      {8.0f, 0, 0, 0, 0},      {16.0f, 0, 0, 0, 0},      {18.0f, 90, 60, 50, 60},
      {20.0f, 18, 22, 70, 140}, {24.01f, 18, 22, 70, 140},
  };
  constexpr int kCount = sizeof(kKeys) / sizeof(kKeys[0]);
  while (hour < 0.0f) hour += 24.0f;
  while (hour >= 24.0f) hour -= 24.0f;

  for (int i = 0; i + 1 < kCount; ++i) {
    const Key a = kKeys[i];
    const Key b = kKeys[i + 1];
    if (hour < a.h || hour > b.h) continue;
    const float t = b.h > a.h ? (hour - a.h) / (b.h - a.h) : 0.0f;
    auto lerp = [t](int x, int y) {
      return static_cast<unsigned char>(static_cast<float>(x) +
                                        (static_cast<float>(y) - static_cast<float>(x)) * t);
    };
    return Color{lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a)};
  }
  return Color{0, 0, 0, 0};
}

}  // namespace bh
