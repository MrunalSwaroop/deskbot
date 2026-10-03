#pragma once

struct DashboardCapabilities {
  bool faceControls;
  bool motionControls;
  bool cameraPreview;
  bool microphoneMonitor;
  bool audioTest;
  bool otaStatus;
};

inline DashboardCapabilities deskbotDashboardCapabilities() {
  return {true, true, true, true, true, true};
}
