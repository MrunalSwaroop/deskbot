#pragma once

#include <Arduino.h>

struct OtaVersionChoice {
  String version;
  String firmwareUrl;
  bool explicitTarget;
};

inline OtaVersionChoice makeOtaVersionChoice(const String &version, const String &firmwareUrl, bool explicitTarget) {
  return {version, firmwareUrl, explicitTarget};
}
