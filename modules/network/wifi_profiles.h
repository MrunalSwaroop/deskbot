#pragma once

#include <Arduino.h>

struct WifiProfile {
  String ssid;
  String password;
};

inline bool wifiProfileConfigured(const WifiProfile &profile) {
  return profile.ssid.length() > 0;
}
