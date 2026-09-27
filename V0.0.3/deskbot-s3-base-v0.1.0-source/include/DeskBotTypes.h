#pragma once

#include <Arduino.h>

enum class NetworkMode : uint8_t {
  Station,
  SetupAccessPoint,
};

struct NetworkStatus {
  NetworkMode mode;
  String ssid;
  String ip;
  int32_t rssi;
};
