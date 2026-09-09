#pragma once

#include <raylib.h>

namespace bh {

// T-ART-02: zoom snap — art is validated at {1, 1.5, 2}; non-integer zoom
// shimmers pixel art. Boundaries: <1.25 -> 1, <1.75 -> 1.5, else 2.
inline float snapZoom(float z) {
  if (z < 1.25f) return 1.0f;
  if (z < 1.75f) return 1.5f;
  return 2.0f;
}

// Era camera (T-007 polish): follows the hero, MMB/arrow-key free-pan,
// Space/MMB-release re-lock, stepped Z zoom + wheel fine zoom, always
// zooming at the cursor, all clamped to the map bounds.
struct CameraRig {
  Camera2D cam{};
  bool manualPan = false;

  void init(float viewW, float viewH) {
    cam.offset = Vector2{viewW * 0.5f, viewH * 0.5f};
    cam.target = Vector2{0.0f, 0.0f};
    cam.rotation = 0.0f;
    cam.zoom = 1.0f;
  }

  void setBounds(float x0, float y0, float x1, float y1, float marginPx) {
    bx0_ = x0 - marginPx;
    by0_ = y0 - marginPx;
    bx1_ = x1 + marginPx;
    by1_ = y1 + marginPx;
    hasBounds_ = true;
  }

  void follow(Vector2 world) {
    if (manualPan) return;
    cam.target = world;
    clampTarget();
  }

  void input() {
    constexpr float kPanPxPerSec = 480.0f;
    const float dt = GetFrameTime();
    int ax = 0;
    int ay = 0;
    if (IsKeyDown(KEY_RIGHT)) ax += 1;
    if (IsKeyDown(KEY_LEFT)) ax -= 1;
    if (IsKeyDown(KEY_DOWN)) ay += 1;
    if (IsKeyDown(KEY_UP)) ay -= 1;
    if (ax != 0 || ay != 0) {
      manualPan = true;
      cam.target.x += static_cast<float>(ax) * kPanPxPerSec * dt / cam.zoom;
      cam.target.y += static_cast<float>(ay) * kPanPxPerSec * dt / cam.zoom;
      clampTarget();
    }

    if (IsKeyPressed(KEY_Z)) {
      const Vector2 center{cam.offset.x, cam.offset.y};  // screen center
      zoomAt(center, cam.zoom < 1.5f ? 2.0f : 1.0f);
    }
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
      zoomAt(GetMousePosition(), snapZoom(cam.zoom + wheel * 0.25f));
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) manualPan = true;
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) && manualPan) {
      const Vector2 d = GetMouseDelta();
      cam.target.x -= d.x / cam.zoom;
      cam.target.y -= d.y / cam.zoom;
      clampTarget();
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE) || IsKeyPressed(KEY_SPACE)) {
      manualPan = false;
    }
  }

 private:
  bool hasBounds_ = false;
  float bx0_ = 0, by0_ = 0, bx1_ = 0, by1_ = 0;

  void clampTarget() {
    if (!hasBounds_) return;
    if (cam.target.x < bx0_) cam.target.x = bx0_;
    if (cam.target.x > bx1_) cam.target.x = bx1_;
    if (cam.target.y < by0_) cam.target.y = by0_;
    if (cam.target.y > by1_) cam.target.y = by1_;
  }

  // Re-anchor so the world point under screenPt stays put across the zoom.
  void zoomAt(Vector2 screenPt, float newZoom) {
    const Vector2 w = GetScreenToWorld2D(screenPt, cam);
    cam.zoom = newZoom;
    cam.target.x = w.x - (screenPt.x - cam.offset.x) / newZoom;
    cam.target.y = w.y - (screenPt.y - cam.offset.y) / newZoom;
    clampTarget();
  }
};

}  // namespace bh
