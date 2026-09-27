#pragma once

#include <Arduino.h>
#include <stddef.h>

class OtaService {
 public:
  void begin();
  void update();
  String boardId() const;

 private:
  bool fetchManifestVersion(char* version, size_t versionSize, bool& allowed);
  void tryRemoteUpdate();

  char ssid_[33] = {};
  char password_[65] = {};
  bool wifiAttempted_ = false;
  bool wifiConnected_ = false;
  bool otaChecked_ = false;
  bool manifestChecked_ = false;
  bool manifestAllowed_ = true;
  unsigned long lastWifiRetryMs_ = 0;
};
