#pragma once

#include <stddef.h>

class OtaService {
 public:
  void begin();
  void update();

 private:
  bool fetchManifestVersion(char* version, size_t versionSize);
  void tryRemoteUpdate();

  char ssid_[33] = {};
  char password_[65] = {};
  bool wifiAttempted_ = false;
  bool wifiConnected_ = false;
  bool otaChecked_ = false;
  bool manifestChecked_ = false;
  unsigned long lastWifiRetryMs_ = 0;
};
