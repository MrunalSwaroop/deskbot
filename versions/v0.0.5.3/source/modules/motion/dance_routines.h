#pragma once

struct DanceRoutine {
  const char* id;
  unsigned long maxDurationMs;
};

inline DanceRoutine rockyDanceRoutine() {
  return {"rocky", 12000UL};
}
