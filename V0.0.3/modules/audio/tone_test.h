#pragma once

struct AudioToneRequest {
  int frequencyHz;
  unsigned long durationMs;
};

inline AudioToneRequest boundedToneRequest(int frequencyHz, unsigned long durationMs) {
  if (frequencyHz < 100) frequencyHz = 100;
  if (frequencyHz > 4000) frequencyHz = 4000;
  if (durationMs > 10000UL) durationMs = 10000UL;
  return {frequencyHz, durationMs};
}
