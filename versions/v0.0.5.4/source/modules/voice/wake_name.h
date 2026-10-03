#pragma once

#include <Arduino.h>

// This is intentionally a policy boundary, not a speech recognizer.
// Raw PDM amplitude cannot identify spoken words. A future local wake-word
// engine or Xiaozhi/relay adapter should call engage() after recognizing the name.
struct WakeNamePolicy {
  String name;
  bool enabled;
};

inline WakeNamePolicy makeWakeNamePolicy(const String &name, bool enabled) {
  WakeNamePolicy policy{name, enabled};
  return policy;
}
