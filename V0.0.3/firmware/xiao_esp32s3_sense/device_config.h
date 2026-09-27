#pragma once

// Optional local files are intentionally ignored by the release package.
#if __has_include("secrets.h")
#include "secrets.h"
#endif

#if __has_include("ota_target.h")
#include "ota_target.h"
#endif

// OTA is disabled in normal local builds. The final bootstrap and GitHub
// release builds explicitly enable it through ota_target.h.
#ifndef ROCKY_OTA_ENABLED
#define ROCKY_OTA_ENABLED 0
#endif

#ifndef DEVICE_ID
#define DEVICE_ID "XIAO-ESP32S3"
#endif

#ifndef APP_VERSION
#define APP_VERSION "1.1.0-usb"
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

// OTA URLs are inert unless ROCKY_OTA_ENABLED is explicitly set to 1.
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
