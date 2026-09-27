#pragma once

struct SenseCameraCapability {
  bool enabled;
  bool psramRequired;
};

inline SenseCameraCapability xiaoSenseCameraCapability() {
  return {true, true};
}
