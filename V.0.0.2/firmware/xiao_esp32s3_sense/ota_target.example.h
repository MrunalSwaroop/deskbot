#pragma once

// Copy this file to ota_target.h only after the deskbot GitHub Pages workflow is ready.
// Keep ota_target.h local and ignored.
#define ROCKY_OTA_ENABLED 1
#define APP_VERSION "1.1.0-usb"
#define OTA_UPDATE_URL "https://YOUR_OWNER.github.io/deskbot/ota/XIAO-ESP32S3.bin"
#define OTA_MANIFEST_URL "https://YOUR_OWNER.github.io/deskbot/ota/xiao-esp32s3-manifest.json"
#define OTA_MANIFEST_HOST "YOUR_OWNER.github.io"
#define OTA_MANIFEST_PATH "/deskbot/ota/xiao-esp32s3-manifest.json"
#define OTA_TARGET_VERSION ""
