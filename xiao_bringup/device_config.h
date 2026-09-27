#pragma once

// Optional local files are intentionally ignored by the release package.
#if __has_include("secrets.h")
#include "secrets.h"
#endif

#if __has_include("ota_target.h")
#include "ota_target.h"
#endif

#ifndef DEVICE_ID
#define DEVICE_ID "XIAO-ESP32S3"
#endif

#ifndef APP_VERSION
#define APP_VERSION "0.3.0-xiao-adjacent"
#endif

#ifndef BLINK_INTERVAL_MS
#define BLINK_INTERVAL_MS 5000UL
#endif

// Wi-Fi remains provisioned through the local setup AP and NVS.
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

// OTA is inert until ota_target.h is created locally or by CI.
#ifndef OTA_MANIFEST_URL
#define OTA_MANIFEST_URL ""
#endif
#ifndef OTA_MANIFEST_HOST
#define OTA_MANIFEST_HOST ""
#endif
#ifndef OTA_MANIFEST_PATH
#define OTA_MANIFEST_PATH ""
#endif
#ifndef OTA_UPDATE_URL
#define OTA_UPDATE_URL ""
#endif
#ifndef OTA_TARGET_VERSION
#define OTA_TARGET_VERSION ""
#endif

#ifndef WIFI_FIRMWARE_LATEST_VERSION
#define WIFI_FIRMWARE_LATEST_VERSION "0.0.0"
#endif
