#pragma once

#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WiFi.h>

#include "DeskBotTypes.h"

class NetworkManager {
 public:
  void begin();
  void loop();
  NetworkStatus status() const;
  bool isSetupMode() const;
  bool saveCredentials(const String &ssid, const String &password);
  void clearCredentials();

 private:
  void startSetupAccessPoint();
  bool connectSavedNetwork();

  Preferences preferences_;
  DNSServer dnsServer_;
  NetworkMode mode_ = NetworkMode::SetupAccessPoint;
  String ssid_;
};
