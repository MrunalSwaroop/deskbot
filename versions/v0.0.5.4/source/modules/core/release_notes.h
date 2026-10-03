#pragma once
#include <stdint.h>
// Release notes are intentionally data-only so the dashboard can show what
// changed without coupling release documentation to hardware drivers.
#define DESKBOT_RELEASE_VERSION "0.0.5.4"
#define DESKBOT_RELEASE_TITLE "Mobile dashboard and responsive OTA startup"
#define DESKBOT_RELEASE_SUMMARY "This release makes local control practical on phones and lets the dashboard become available before the OTA check begins."
static const char *const DESKBOT_RELEASE_CHANGES[] = {
  "Responsive phone-first dashboard with touch-sized controls and quick navigation",
  "Grouped face, personality, motion, audio, camera, Wi-Fi, and OTA controls into mobile-friendly cards",
  "Published-version history panel populated from the OTA catalog",
  "OTA HTTPS manifest check deferred four seconds after boot so the dashboard starts first",
  "Visible explanation that an active OTA download temporarily pauses normal firmware work",
  "Release documentation now records the full version-by-version change history"
};
static const uint8_t DESKBOT_RELEASE_CHANGE_COUNT =
    sizeof(DESKBOT_RELEASE_CHANGES) / sizeof(DESKBOT_RELEASE_CHANGES[0]);
