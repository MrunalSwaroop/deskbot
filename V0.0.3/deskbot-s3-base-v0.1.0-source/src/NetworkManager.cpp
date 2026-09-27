#include "NetworkManager.h"

#include <ESPmDNS.h>

#include "DeskBotConfig.h"

void NetworkManager::begin() {
  preferences_.begin("deskbot", false);
  ssid_ = preferences_.getString("wifi_ssid", "");

  if (!ssid_.isEmpty() && connectSavedNetwork()) {
    mode_ = NetworkMode::Station;
    MDNS.begin(DeskBotConfig::MDNS_HOSTNAME);
    Serial.printf("[network] Connected to %s at %s\n", ssid_.c_str(), WiFi.localIP().toString().c_str());
    return;
  }

  startSetupAccessPoint();
}

void NetworkManager::loop() {
  if (mode_ == NetworkMode::SetupAccessPoint) {
    dnsServer_.processNextRequest();
  }
}

NetworkStatus NetworkManager::status() const {
  NetworkStatus result;
  result.mode = mode_;
  result.ssid = mode_ == NetworkMode::Station ? WiFi.SSID() : DeskBotConfig::SETUP_AP_SSID;
  result.ip = mode_ == NetworkMode::Station ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  result.rssi = mode_ == NetworkMode::Station ? WiFi.RSSI() : 0;
  return result;
}

bool NetworkManager::isSetupMode() const {
  return mode_ == NetworkMode::SetupAccessPoint;
}

bool NetworkManager::saveCredentials(const String &ssid, const String &password) {
  if (ssid.isEmpty() || ssid.length() > 32 || password.length() > 63) {
    return false;
  }

  preferences_.putString("wifi_ssid", ssid);
  preferences_.putString("wifi_pass", password);
  return true;
}

void NetworkManager::clearCredentials() {
  preferences_.remove("wifi_ssid");
  preferences_.remove("wifi_pass");
}

bool NetworkManager::connectSavedNetwork() {
  const String password = preferences_.getString("wifi_pass", "");
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid_.c_str(), password.c_str());

  constexpr uint32_t timeoutMs = 15000;
  const uint32_t started = millis();
  Serial.printf("[network] Connecting to saved Wi-Fi SSID: %s", ssid_.c_str());

  while (WiFi.status() != WL_CONNECTED && millis() - started < timeoutMs) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

void NetworkManager::startSetupAccessPoint() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(DeskBotConfig::SETUP_AP_SSID, DeskBotConfig::SETUP_AP_PASSWORD);
  dnsServer_.start(53, "*", WiFi.softAPIP());
  mode_ = NetworkMode::SetupAccessPoint;
  Serial.printf("[network] Setup AP started: %s (password: %s), IP: %s\n",
                DeskBotConfig::SETUP_AP_SSID,
                DeskBotConfig::SETUP_AP_PASSWORD,
                WiFi.softAPIP().toString().c_str());
}
