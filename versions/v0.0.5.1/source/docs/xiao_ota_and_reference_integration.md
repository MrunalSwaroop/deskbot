# XIAO OTA and Reference Integration

The active firmware is `firmware/xiao_esp32s3_sense/xiao_esp32s3_sense.ino`. Its HTTPS OTA service uses Wi-Fi credentials stored by the local setup page and checks a semantic-versioned manifest once at boot.

## First USB bootstrap

Use the settings in `boards/xiao-esp32s3-sense/README.md`, choose an OTA-capable partition scheme, enable `OPI PSRAM`, and upload over USB. Keep `firmware/xiao_esp32s3_sense/ota_target.h` absent for a USB-only local build.

## Pages release

Push the Deskbot root to the public `MrunalSwaroop/deskbot` repository and configure GitHub Pages to use Actions. `publish-ota.yml` creates the ignored target header during CI, compiles the image, and publishes:

```text
https://MrunalSwaroop.github.io/deskbot/ota/XIAO-ESP32S3.bin
https://MrunalSwaroop.github.io/deskbot/ota/xiao-esp32s3-manifest.json
```

The workflow derives the actual repository owner and name, so a fork uses its own Pages URL.

## Fleet behavior

The manifest carries device type, semantic version, rollout mode, selected IDs, firmware URL, and source commit. `fleet/selected_boards.json` controls whether every eligible XIAO board or only selected IDs may update.

## Recovery

USB remains the recovery path. If a board appears to reboot without reaching the application, hold BOOT, tap RESET, release BOOT, and upload a known-good USB image. Do not repeatedly reset a working board while an OTA download is in progress.

## Reference layers

The desktop-pet OLED reference is represented by display-only modules under `modules/faces/`. The earlier CI/CD reference informed the manifest, semantic-version, HTTPS, and GitHub Pages design. Neither reference adds a mandatory service dependency to the active Deskbot controller.
