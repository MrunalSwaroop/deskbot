# Changelog

All notable changes to the modular DeskBot-S3 base are documented here.

## [0.1.0] — 2026-09-27

### Added
- PlatformIO target for Seeed Studio XIAO ESP32-S3 (8 MB flash).
- 8 MB partition map with two 3.5 MB OTA-capable application slots.
- Browser-flashable merged factory image at offset `0x0`.
- Offline/local captive Wi-Fi provisioning portal with removable stored credentials.
- Local HTTP administration dashboard for status, Wi-Fi updates, I²C scan, and optional Sense-camera capture.
- Safe motor test controller using XIAO GPIO 1–4 with boot-off state and a 500 ms automatic command timeout.
- Optional XIAO ESP32-S3 Sense OV2640 camera initialization using Espressif’s official pin map.
- Hardware connection, power-protection, and staged-build documentation.

### Deliberate boundaries
- VL53L5CX, ISM330DHCX, and MMC5983MA runtime drivers are intentionally postponed until each physical module is confirmed by the I²C diagnostic. This keeps the first release usable without every optional sensor.

### Security / operations notes
- Wi-Fi credentials stay on the microcontroller; no cloud backend is used.
- The setup access point uses a documented development password (`deskbot-s3`). Change this in `include/DeskBotConfig.h` before any public deployment.
