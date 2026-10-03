#pragma once

struct PdmMicrophoneCapability {
  int clockPin;
  int dataPin;
  int sampleRate;
};

inline PdmMicrophoneCapability xiaoSensePdmMicrophone(int clockPin, int dataPin) {
  return {clockPin, dataPin, 16000};
}
