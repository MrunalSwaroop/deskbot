#include "ota_service.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <cstring>
#include "device_config.h"

#if defined(ARDUINO_UNOR4_WIFI)
#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <OTAUpdate.h>
#include "ota_pages_root_ca.h"
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Preferences.h>
#include "ota_pages_root_ca.h"
#endif

namespace {
constexpr unsigned long kWifiRetryIntervalMs = 10000UL;
constexpr uint32_t kCredentialsMagic = 0x46504254UL;

struct StoredCredentials {
  uint32_t magic;
  char ssid[33];
  char password[65];
};

bool validCredentials(const StoredCredentials& stored) {
  return stored.magic == kCredentialsMagic && stored.ssid[0] != '\0';
}

bool parseVersion(const char* text, int& major, int& minor, int& patch, int& build) {
  return text != nullptr && sscanf(text, "%d.%d.%d.%d", &major, &minor, &patch, &build) == 4;
}

bool isNewerVersion(const char* candidate, const char* current) {
  int candidateMajor = 0;
  int candidateMinor = 0;
  int candidatePatch = 0;
  int candidateBuild = 0;
  int currentMajor = 0;
  int currentMinor = 0;
  int currentPatch = 0;
  int currentBuild = 0;

  if (!parseVersion(candidate, candidateMajor, candidateMinor, candidatePatch, candidateBuild) ||
      !parseVersion(current, currentMajor, currentMinor, currentPatch, currentBuild)) {
    return false;
  }

  if (candidateMajor != currentMajor) {
    return candidateMajor > currentMajor;
  }
  if (candidateMinor != currentMinor) {
    return candidateMinor > currentMinor;
  }
  if (candidatePatch != currentPatch) {
    return candidatePatch > currentPatch;
  }
  return candidateBuild > currentBuild;
}

String currentBoardId() {
#if defined(ARDUINO_ARCH_ESP32)
  const uint64_t chip = ESP.getEfuseMac();
  char id[32];
  snprintf(id, sizeof(id), "XIAO-%04lX%08lX",
           static_cast<unsigned long>(chip >> 32),
           static_cast<unsigned long>(chip & 0xFFFFFFFFULL));
  return String(id);
#else
  return String(DEVICE_ID);
#endif
}

bool manifestAllowsBoard(const String& body) {
  const String board = currentBoardId();
  const int rolloutKey = body.indexOf("\"rollout\"");
  if (rolloutKey < 0) return true;

  const int colon = body.indexOf(':', rolloutKey);
  const int firstQuote = body.indexOf('\"', colon);
  const int secondQuote = body.indexOf('\"', firstQuote + 1);
  if (colon < 0 || firstQuote < 0 || secondQuote <= firstQuote) return true;

  const String rollout = body.substring(firstQuote + 1, secondQuote);
  if (rollout == "all") return true;
  if (rollout != "selected") return false;

  const int boardsKey = body.indexOf("\"boards\"");
  if (boardsKey < 0) return false;
  const int arrayStart = body.indexOf('[', boardsKey);
  const int arrayEnd = body.indexOf(']', arrayStart);
  if (arrayStart < 0 || arrayEnd <= arrayStart) return false;
  const String selected = body.substring(arrayStart, arrayEnd + 1);
  return selected.indexOf("\"all\"") >= 0 ||
         selected.indexOf("\"" + board + "\"") >= 0;
}
}

String OtaService::boardId() const {
  return currentBoardId();
}

void OtaService::setUiCallback(OtaUiCallback callback) {
  uiCallback_ = callback;
}

OtaState OtaService::state() const {
  return state_;
}

const char *OtaService::stateName() const {
  switch (state_) {
    case OtaState::Disabled: return "disabled";
    case OtaState::Idle: return "idle";
    case OtaState::Checking: return "checking";
    case OtaState::UpToDate: return "up_to_date";
    case OtaState::UpdateAvailable: return "update_available";
    case OtaState::Downloading: return "downloading";
    case OtaState::Installing: return "installing";
    case OtaState::Rebooting: return "rebooting";
    case OtaState::NotSelected: return "not_selected";
    case OtaState::Error: return "error";
  }
  return "unknown";
}

const char *OtaService::remoteVersion() const {
  return remoteVersion_;
}

const char *OtaService::lastError() const {
  return lastError_;
}

int OtaService::progress() const {
  return progress_;
}

bool OtaService::updateAvailable() const {
  return updateAvailable_;
}
const char *OtaService::targetVersion() const {
  return targetVersion_;
}
const char *OtaService::catalogUrl() const {
  return OTA_CATALOG_URL;
}
bool OtaService::setTargetVersion(const String &version) {
  int major = 0, minor = 0, patch = 0, build = 0;
  if (!parseVersion(version.c_str(), major, minor, patch, build)) return false;
  version.toCharArray(targetVersion_, sizeof(targetVersion_));
  Preferences preferences;
  preferences.begin("rocky-xiao", false);
  preferences.putString("otaTarget", version);
  preferences.end();
  updateAvailable_ = true;
  setState(OtaState::UpdateAvailable, 0);
  return true;
}
void OtaService::clearTargetVersion() {
  targetVersion_[0] = '\0';
  Preferences preferences;
  preferences.begin("rocky-xiao", false);
  preferences.remove("otaTarget");
  preferences.end();
}
bool OtaService::fetchCatalog(String &catalog) {
#if defined(ARDUINO_ARCH_ESP32)
  if (OTA_CATALOG_URL[0] == '\0') return false;
  NetworkClientSecure client;
  client.setHandshakeTimeout(10);
  client.setCACert(pages_root_ca);
  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(10000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  if (!http.begin(client, OTA_CATALOG_URL)) return false;
  const int statusCode = http.GET();
  if (statusCode != HTTP_CODE_OK) { http.end(); return false; }
  catalog = http.getString();
  http.end();
  return catalog.length() > 0;
#else
  (void)catalog;
  return false;
#endif
}
void OtaService::setState(OtaState state, int progress) {
  state_ = state;
  if (progress >= 0) progress_ = constrain(progress, 0, 100);
  Serial.print("OTA state: ");
  Serial.print(stateName());
  if (state_ == OtaState::Downloading || state_ == OtaState::Installing) {
    Serial.print(" ");
    Serial.print(progress_);
    Serial.print("%");
  }
  Serial.println();
  if (uiCallback_) uiCallback_(stateName(), progress_);
}

void OtaService::setError(const String &message) {
  message.toCharArray(lastError_, sizeof(lastError_));
  setState(OtaState::Error, 0);
}

void OtaService::begin() {
#if !ROCKY_OTA_ENABLED
  setState(OtaState::Disabled, 0);
  Serial.println("OTA bootstrap: disabled in this local USB build");
  return;
#endif
#if defined(ARDUINO_UNOR4_WIFI)
  Serial.println("OTA bootstrap: starting Wi-Fi service");

  StoredCredentials stored{};
  EEPROM.get(0, stored);

  if (validCredentials(stored)) {
    strncpy(ssid_, stored.ssid, sizeof(ssid_) - 1);
    strncpy(password_, stored.password, sizeof(password_) - 1);
    Serial.println("OTA bootstrap: using Wi-Fi credentials stored on board");
  } else if (WIFI_SSID[0] != '\0') {
    stored.magic = kCredentialsMagic;
    strncpy(stored.ssid, WIFI_SSID, sizeof(stored.ssid) - 1);
    strncpy(stored.password, WIFI_PASSWORD, sizeof(stored.password) - 1);
    EEPROM.put(0, stored);
    strncpy(ssid_, stored.ssid, sizeof(ssid_) - 1);
    strncpy(password_, stored.password, sizeof(password_) - 1);
    Serial.println("OTA bootstrap: stored Wi-Fi credentials on board");
  } else {
    Serial.println("OTA bootstrap: no Wi-Fi credentials stored; create secrets.h for first USB setup");
    return;
  }

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("OTA bootstrap: Wi-Fi module not detected");
    return;
  }

  const String wifiFirmwareVersion = WiFi.firmwareVersion();
  Serial.print("OTA bootstrap: Wi-Fi bridge firmware=");
  Serial.println(wifiFirmwareVersion);
  if (wifiFirmwareVersion < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.print("OTA bootstrap: upgrade bridge firmware; recommended minimum=");
    Serial.println(WIFI_FIRMWARE_LATEST_VERSION);
  }

  lastWifiRetryMs_ = 0;
  update();
#elif defined(ARDUINO_ARCH_ESP32)
  Serial.println("OTA bootstrap: starting XIAO ESP32-S3 Wi-Fi service");

  Preferences preferences;
  // Reuse the bring-up firmware's existing NVS namespace and key names.
  // This keeps OTA additive: provisioning from /wifi remains the source of truth.
  preferences.begin("rocky-xiao", false);
  const String storedSsid = preferences.getString("ssid", "");
  const String storedPassword = preferences.getString("pass", "");
  String storedTarget = preferences.getString("otaTarget", "");
  if (storedTarget.length() == 0 && OTA_TARGET_VERSION[0] != '\0') storedTarget = OTA_TARGET_VERSION;
  storedTarget.toCharArray(targetVersion_, sizeof(targetVersion_));

  if (storedSsid.length() > 0) {
    storedSsid.toCharArray(ssid_, sizeof(ssid_));
    storedPassword.toCharArray(password_, sizeof(password_));
    Serial.println("OTA bootstrap: using Wi-Fi credentials stored on board");
  } else if (WIFI_SSID[0] != '\0') {
    strncpy(ssid_, WIFI_SSID, sizeof(ssid_) - 1);
    strncpy(password_, WIFI_PASSWORD, sizeof(password_) - 1);
    preferences.putString("ssid", ssid_);
    preferences.putString("pass", password_);
    Serial.println("OTA bootstrap: stored Wi-Fi credentials on board");
  } else {
    Serial.println("OTA bootstrap: no Wi-Fi credentials stored; create secrets.h for first USB setup");
    preferences.end();
    return;
  }
  preferences.end();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected_ = true;
    Serial.print("OTA bootstrap: using active Wi-Fi connection, IP=");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.mode(WIFI_STA);
  }
  lastWifiRetryMs_ = 0;
  // Do not block the critical boot path with HTTPS/TLS or a firmware download.
  // The main loop will perform the check after the dashboard is responsive.
  checkAfterMs_ = millis() + 4000UL;
  Serial.println("OTA bootstrap: check deferred 4 seconds so dashboard can start");
#else
  Serial.println("OTA bootstrap: Wi-Fi service not enabled for this board yet");
#endif
}

void OtaService::update() {
#if !ROCKY_OTA_ENABLED
  return;
#endif
  if (checkAfterMs_ != 0) {
    if ((long)(millis() - checkAfterMs_) < 0) return;
    checkAfterMs_ = 0;
  }
#if defined(ARDUINO_UNOR4_WIFI)
  if (ssid_[0] == '\0' || otaChecked_) {
    return;
  }

  if (wifiConnected_) {
    tryRemoteUpdate();
    return;
  }

  const unsigned long nowMs = millis();
  if (wifiAttempted_ && nowMs - lastWifiRetryMs_ < kWifiRetryIntervalMs) {
    return;
  }

  wifiAttempted_ = true;
  lastWifiRetryMs_ = nowMs;

  Serial.print("OTA bootstrap: connecting to ");
  Serial.println(ssid_);

  if (WiFi.begin(ssid_, password_) == WL_CONNECTED) {
    Serial.println("OTA bootstrap: Wi-Fi link connected; waiting for DHCP");

    const unsigned long dhcpStartMs = millis();
    while (millis() - dhcpStartMs < 15000UL) {
      const IPAddress ip = WiFi.localIP();
      if (ip[0] != 0 || ip[1] != 0 || ip[2] != 0 || ip[3] != 0) {
        wifiConnected_ = true;
        Serial.print("OTA bootstrap: Wi-Fi connected, IP=");
        Serial.println(ip);
        Serial.println("OTA bootstrap: remote download will be enabled in the next step");
        return;
      }
      delay(250);
    }

    Serial.println("OTA bootstrap: DHCP did not assign an IP address yet");
  } else {
    Serial.println("OTA bootstrap: Wi-Fi connection attempt did not complete");
  }
#elif defined(ARDUINO_ARCH_ESP32)
  if (ssid_[0] == '\0' || otaChecked_) {
    return;
  }

  if (wifiConnected_) {
    tryRemoteUpdate();
    return;
  }

  const unsigned long nowMs = millis();
  if (wifiAttempted_ && nowMs - lastWifiRetryMs_ < kWifiRetryIntervalMs) {
    return;
  }

  wifiAttempted_ = true;
  lastWifiRetryMs_ = nowMs;
  Serial.print("OTA bootstrap: connecting to ");
  Serial.println(ssid_);
  WiFi.begin(ssid_, password_);

  const unsigned long connectStartMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - connectStartMs < 15000UL) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected_ = true;
    Serial.print("OTA bootstrap: Wi-Fi connected, IP=");
    Serial.println(WiFi.localIP());
    return;
  }

  Serial.println("OTA bootstrap: Wi-Fi connection attempt did not complete");
#else
  // Other board backends will be added after the UNO bootstrap is verified.
#endif
}

bool OtaService::fetchManifestVersion(char* version, size_t versionSize, bool& allowed) {
#if defined(ARDUINO_UNOR4_WIFI)
  if (OTA_MANIFEST_HOST[0] == '\0' || OTA_MANIFEST_PATH[0] == '\0') {
    return false;
  }

  WiFiSSLClient client;
  if (!client.connect(OTA_MANIFEST_HOST, 443)) {
    Serial.println("OTA bootstrap: manifest connection failed");
    return false;
  }

  client.print("GET ");
  client.print(OTA_MANIFEST_PATH);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(OTA_MANIFEST_HOST);
  client.println("Connection: close");
  client.println();

  const unsigned long requestStartMs = millis();
  while (!client.available() && client.connected() &&
         millis() - requestStartMs < 10000UL) {
    delay(10);
  }

  if (!client.available()) {
    client.stop();
    Serial.println("OTA bootstrap: manifest response timeout");
    return false;
  }

  const int statusCode = client.parseInt();
  client.readStringUntil('\n');
  if (statusCode != 200) {
    client.stop();
    Serial.print("OTA bootstrap: manifest HTTP status=");
    Serial.println(statusCode);
    return false;
  }

  while (client.connected()) {
    const String headerLine = client.readStringUntil('\n');
    if (headerLine == "\r" || headerLine.length() <= 1) {
      break;
    }
  }

  String body;
  const unsigned long bodyStartMs = millis();
  while (millis() - bodyStartMs < 10000UL) {
    while (client.available()) {
      body += static_cast<char>(client.read());
    }
    if (!client.connected() && !client.available()) {
      break;
    }
    delay(10);
  }
  client.stop();

  const int versionKey = body.indexOf("\"version\"");
  const int colon = body.indexOf(':', versionKey);
  const int firstQuote = body.indexOf('"', colon);
  const int secondQuote = body.indexOf('"', firstQuote + 1);
  if (versionKey < 0 || colon < 0 || firstQuote < 0 || secondQuote <= firstQuote) {
    Serial.println("OTA bootstrap: manifest version missing");
    return false;
  }

  allowed = manifestAllowsBoard(body);
  const String remoteVersion = body.substring(firstQuote + 1, secondQuote);
  remoteVersion.toCharArray(version, versionSize);
  Serial.print("OTA bootstrap: manifest version=");
  Serial.println(version);
  if (!allowed) {
    Serial.print("OTA bootstrap: board ");
    Serial.print(currentBoardId());
    Serial.println(" is not selected for this release");
  }
  return version[0] != '\0';
#elif defined(ARDUINO_ARCH_ESP32)
  if (OTA_MANIFEST_URL[0] == '\0') {
    return false;
  }

  NetworkClientSecure client;
  client.setHandshakeTimeout(10);
  client.setCACert(pages_root_ca);
  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(10000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.useHTTP10(true);
  Serial.println("OTA bootstrap: requesting XIAO HTTPS manifest");
  if (!http.begin(client, OTA_MANIFEST_URL)) {
    Serial.println("OTA bootstrap: manifest connection setup failed");
    return false;
  }

  const int statusCode = http.GET();
  Serial.print("OTA bootstrap: manifest request result=");
  Serial.println(statusCode);
  if (statusCode != HTTP_CODE_OK) {
    Serial.print("OTA bootstrap: manifest HTTP status=");
    Serial.println(statusCode);
    http.end();
    return false;
  }

  const String body = http.getString();
  http.end();

  const int versionKey = body.indexOf("\"version\"");
  const int colon = body.indexOf(':', versionKey);
  const int firstQuote = body.indexOf('"', colon);
  const int secondQuote = body.indexOf('"', firstQuote + 1);
  if (versionKey < 0 || colon < 0 || firstQuote < 0 || secondQuote <= firstQuote) {
    Serial.println("OTA bootstrap: manifest version missing");
    return false;
  }

  allowed = manifestAllowsBoard(body);
  const String remoteVersion = body.substring(firstQuote + 1, secondQuote);
  remoteVersion.toCharArray(version, versionSize);
  Serial.print("OTA bootstrap: manifest version=");
  Serial.println(version);
  if (!allowed) {
    Serial.print("OTA bootstrap: board ");
    Serial.print(currentBoardId());
    Serial.println(" is not selected for this release");
  }
  return version[0] != '\0';
#else
  (void)version;
  (void)versionSize;
  return false;
#endif
}

void OtaService::tryRemoteUpdate() {
#if defined(ARDUINO_UNOR4_WIFI)
  otaChecked_ = true;

  char remoteVersion[24] = {};
  const bool manifestConfigured = OTA_MANIFEST_HOST[0] != '\0' &&
                                  OTA_MANIFEST_PATH[0] != '\0';
  const char* targetVersion = OTA_TARGET_VERSION;

  if (manifestConfigured) {
    if (!fetchManifestVersion(remoteVersion, sizeof(remoteVersion), manifestAllowed_)) {
      return;
    }
    if (!manifestAllowed_) return;
    if (!isNewerVersion(remoteVersion, APP_VERSION)) {
      Serial.print("OTA bootstrap: already running version ");
      Serial.println(APP_VERSION);
      return;
    }
    targetVersion = remoteVersion;
  } else if (OTA_UPDATE_URL[0] == '\0' || OTA_TARGET_VERSION[0] == '\0') {
    Serial.println("OTA bootstrap: no update package configured");
    return;
  } else if (strcmp(APP_VERSION, OTA_TARGET_VERSION) == 0) {
    Serial.print("OTA bootstrap: already running target version ");
    Serial.println(OTA_TARGET_VERSION);
    return;
  }

  Serial.print("OTA bootstrap: downloading version ");
  Serial.println(targetVersion);

  OTAUpdate ota;
  int ret = ota.begin("/update.bin");
  if (ret != OTAUpdate::OTA_ERROR_NONE) {
    Serial.print("OTA bootstrap: ota.begin failed: ");
    Serial.println(ret);
    return;
  }

  ret = ota.setCACert(pages_root_ca);
  if (ret != OTAUpdate::OTA_ERROR_NONE) {
    Serial.print("OTA bootstrap: certificate setup failed: ");
    Serial.println(ret);
    return;
  }

  const int otaSize = ota.download(OTA_UPDATE_URL, "/update.bin");
  if (otaSize <= 0) {
    Serial.print("OTA bootstrap: download failed: ");
    Serial.println(otaSize);
    return;
  }

  Serial.print("OTA bootstrap: package downloaded=");
  Serial.println(otaSize);

  ret = ota.verify();
  if (ret != OTAUpdate::OTA_ERROR_NONE) {
    Serial.print("OTA bootstrap: package verification failed: ");
    Serial.println(ret);
    return;
  }

  ret = ota.update("/update.bin");
  if (ret != OTAUpdate::OTA_ERROR_NONE) {
    Serial.print("OTA bootstrap: update failed: ");
    Serial.println(ret);
    return;
  }

  Serial.println("OTA bootstrap: update accepted; board may reboot now");
#elif defined(ARDUINO_ARCH_ESP32)
  otaChecked_ = true;
  setState(OtaState::Checking, 0);

  char remoteVersion[24] = {};
  String updateUrl = OTA_UPDATE_URL;
  bool explicitTarget = targetVersion_[0] != '\0';

  if (explicitTarget) {
    String catalog;
    if (!fetchCatalog(catalog)) {
      setError("version catalog fetch failed");
      return;
    }
    const String needle = String("\"version\":\"") + targetVersion_ + "\"";
    const int versionPos = catalog.indexOf(needle);
    if (versionPos < 0) {
      setError(String("version not found in catalog: ") + targetVersion_);
      return;
    }
    const int firmwareKey = catalog.indexOf("\"firmware\"", versionPos);
    const int colon = catalog.indexOf(':', firmwareKey);
    const int firstQuote = catalog.indexOf('"', colon);
    const int secondQuote = catalog.indexOf('"', firstQuote + 1);
    if (firmwareKey < 0 || colon < 0 || firstQuote < 0 || secondQuote <= firstQuote) {
      setError("catalog firmware URL missing");
      return;
    }
    updateUrl = catalog.substring(firstQuote + 1, secondQuote);
    strncpy(remoteVersion_, targetVersion_, sizeof(remoteVersion_) - 1);
    remoteVersion_[sizeof(remoteVersion_) - 1] = '\0';
    strncpy(remoteVersion, targetVersion_, sizeof(remoteVersion) - 1);
    remoteVersion[sizeof(remoteVersion) - 1] = '\0';
    manifestAllowed_ = true;
    manifestChecked_ = true;
    Serial.print("OTA bootstrap: explicit target selected="); Serial.println(remoteVersion);
  } else {
    if (!fetchManifestVersion(remoteVersion, sizeof(remoteVersion), manifestAllowed_)) {
      setError("manifest fetch failed");
      return;
    }
    strncpy(remoteVersion_, remoteVersion, sizeof(remoteVersion_) - 1);
    remoteVersion_[sizeof(remoteVersion_) - 1] = '\0';
    manifestChecked_ = true;
    if (!manifestAllowed_) {
      setState(OtaState::NotSelected, 0);
      return;
    }
  }

  if (!explicitTarget && !isNewerVersion(remoteVersion, APP_VERSION)) {
    updateAvailable_ = false;
    setState(OtaState::UpToDate, 100);
    Serial.print("OTA bootstrap: already running version ");
    Serial.println(APP_VERSION);
    return;
  }
  if (explicitTarget && strcmp(remoteVersion, APP_VERSION) == 0) {
    updateAvailable_ = false;
    clearTargetVersion();
    setState(OtaState::UpToDate, 100);
    Serial.println("OTA bootstrap: selected target is already running");
    return;
  }

  updateAvailable_ = true;
  setState(OtaState::UpdateAvailable, 0);
  if (updateUrl.length() == 0) {
    setError("update package URL missing");
    return;
  }

  Serial.print("OTA bootstrap: downloading XIAO firmware version ");
  Serial.println(remoteVersion);
  setState(OtaState::Downloading, 0);

  NetworkClientSecure client;
  client.setHandshakeTimeout(30);
  client.setCACert(pages_root_ca);
  HTTPUpdate updater(120000);
  updater.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  updater.rebootOnUpdate(true);
  updater.onStart([this]() { setState(OtaState::Downloading, 0); });
  updater.onProgress([this](int current, int total) {
    const int percent = total > 0 ? (current * 100) / total : 0;
    setState(OtaState::Downloading, percent);
  });
  updater.onEnd([this]() { setState(OtaState::Installing, 100); });
  updater.onError([this](int error) { setError(String("HTTPUpdate error ") + error); });

  const t_httpUpdate_return result = updater.update(client, updateUrl.c_str(), APP_VERSION);
  if (result == HTTP_UPDATE_OK) {
    clearTargetVersion();
    setState(OtaState::Rebooting, 100);
    Serial.println("OTA bootstrap: XIAO update accepted; board will reboot");
  } else if (result == HTTP_UPDATE_NO_UPDATES) {
    updateAvailable_ = false;
    setState(OtaState::UpToDate, 100);
    Serial.println("OTA bootstrap: server reported no update");
  } else {
    setError(updater.getLastErrorString());
    Serial.print("OTA bootstrap: XIAO update failed: ");
    Serial.println(updater.getLastErrorString());
  }
#endif
}
