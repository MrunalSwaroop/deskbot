#pragma once

// Face modules are display-only. The integration firmware owns the display
// object, state transitions, Wi-Fi, motors, servo, camera, and audio.
struct FaceRenderContext {
  unsigned long frame;
  bool blinking;
  bool lookingLeft;
  bool lookingRight;
};
