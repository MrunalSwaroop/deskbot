#pragma once

#include <stdint.h>

// Hardware-neutral contract for the first microphone/audio integration stage.
// The active XIAO sketch owns I2S objects and calls this policy with PCM blocks.
struct MicLoopbackConfig {
  uint8_t volumePercent;
  uint32_t durationMs;
};

inline MicLoopbackConfig boundMicLoopbackConfig(int volumePercent, uint32_t durationMs) {
  if (volumePercent < 0) volumePercent = 0;
  if (volumePercent > 100) volumePercent = 100;
  if (durationMs < 1000UL) durationMs = 1000UL;
  return {static_cast<uint8_t>(volumePercent), durationMs};
}
