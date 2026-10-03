#pragma once

#include <stdint.h>

// Release notes are intentionally data-only so the dashboard can show what
// changed without coupling release documentation to hardware drivers.
#define DESKBOT_RELEASE_VERSION "0.0.5.3"
#define DESKBOT_RELEASE_TITLE "Dual Wi-Fi, remote OLED, and versioned OTA"
#define DESKBOT_RELEASE_SUMMARY "This release improves network recovery, remote monitoring, and controlled firmware version selection."

static const char *const DESKBOT_RELEASE_CHANGES[] = {
  "Two saved 2.4 GHz Wi-Fi profiles with automatic profile failover",
  "Setup AP at 192.168.4.1 after five minutes without network service",
  "Responsive Wi-Fi configuration page for the normal dashboard and setup AP",
  "Remote OLED mirror at /oled.svg, refreshed from the dashboard",
  "OTA catalog with retained firmware versions for intentional upgrade or downgrade",
  "Persistent ota target <version>, ota latest, and ota clear commands",
  "Existing camera, microphone, speaker, servo, motor, and personality features preserved"
};

static const uint8_t DESKBOT_RELEASE_CHANGE_COUNT =
    sizeof(DESKBOT_RELEASE_CHANGES) / sizeof(DESKBOT_RELEASE_CHANGES[0]);
