#pragma once

#include <Arduino.h>
#include <stddef.h>

enum class OtaState : uint8_t {
  Disabled,
  Idle,
  Checking,
  UpToDate,
  UpdateAvailable,
  Downloading,
  Installing,
  Rebooting,
  NotSelected,
  Error
};

using OtaUiCallback = void (*)(const char *state, int progress);

class OtaService {
 public:
  void begin();
  void update();
  String boardId() const;
  void setUiCallback(OtaUiCallback callback);
  OtaState state() const;
  const char *stateName() const;
  const char *remoteVersion() const;
  const char *targetVersion() const;
  const char *catalogUrl() const;
  const char *lastError() const;
  int progress() const;
  bool updateAvailable() const;
  bool setTargetVersion(const String &version);
  void clearTargetVersion();
  bool fetchCatalog(String &catalog);

 private:
  bool fetchManifestVersion(char* version, size_t versionSize, bool& allowed);
  void tryRemoteUpdate();
  void setState(OtaState state, int progress = -1);
  void setError(const String &message);

  char ssid_[33] = {};
  char password_[65] = {};
  char remoteVersion_[24] = {};
  char targetVersion_[24] = {};
  char lastError_[96] = {};
  bool wifiAttempted_ = false;
  bool wifiConnected_ = false;
  bool otaChecked_ = false;
  bool manifestChecked_ = false;
  bool manifestAllowed_ = true;
  bool updateAvailable_ = false;
  int progress_ = 0;
  OtaState state_ = OtaState::Idle;
  OtaUiCallback uiCallback_ = nullptr;
  unsigned long lastWifiRetryMs_ = 0;
  unsigned long checkAfterMs_ = 0;
};
