# Deskbot — Modular XIAO ESP32-S3 Desk Robot

This repository is the active Deskbot project. The intended Windows working root is:

```text
C:\Users\msperavali\OneDrive - VE Commercial Vehicles Ltd\Mrunal\Deskbot
```

The active controller is the **Seeed XIAO ESP32-S3 Sense**. Historical UNO and generic ESP32 experiments are retained under `legacy/` and are not part of the active XIAO build.

The active release being prepared is **v0.0.5.3**, based on the saved v0.0.5.2 firmware. It adds dual Wi-Fi fallback, remote OLED monitoring, a retained OTA version catalog, and a dashboard change log.

## What this release provides

The current XIAO firmware drives the OLED face, pan servo, two-channel DRV8833 with two N90 motors, Sense camera, onboard PDM microphone monitor, MAX98357A I2S tone and microphone loopback output, Wi-Fi provisioning, local dashboard, personalities, face states, head-state behavior, dance, and manifest-based OTA.

The repository is organized so that character art, personality behavior, motion, audio, voice, camera, sensing, networking, board profiles, and OTA policy can be added or removed without scattering hardware assumptions across the project.

## Clean repository layout

```text
Deskbot/
├── boards/xiao-esp32s3-sense/
│   ├── board.json
│   ├── pinmap.h
│   └── README.md
├── firmware/xiao_esp32s3_sense/
│   ├── xiao_esp32s3_sense.ino
│   ├── device_config.h
│   ├── ota_service.cpp
│   ├── ota_service.h
│   ├── ota_pages_root_ca.h
│   ├── ota_target.example.h
│   └── README.md
├── modules/
│   ├── core/        ├── faces/        ├── personalities/
│   ├── motion/      ├── audio/        ├── voice/
│   ├── vision/      ├── sensing/      ├── network/  └── ota/
├── hardware/wiring/
├── fleet/
├── tools/
├── legacy/
├── .github/workflows/
├── VERSION
└── CHANGE_SUMMARY.md
```

## Initial USB preparation

The current v0.0.1 base can be checked over USB, but OTA requires one final USB-uploaded OTA bootstrap. Follow [`docs/BASELINE_V0.0.1.md`](docs/BASELINE_V0.0.1.md) for that one-time step. Afterward, normal releases are GitHub-only; USB remains recovery-only.

| Arduino IDE setting | Value |
|---|---|
| Board | `XIAO_ESP32S3` |
| Flash Size | `8MB (64Mb)` |
| PSRAM | `OPI PSRAM` |
| Partition Scheme | OTA-capable `Default with spiffs` |
| Upload Mode | `UART0 / Hardware CDC` |
| USB Mode | `Hardware CDC and JTAG` |
| Serial Monitor | `115200 baud` |

Keep the motor supply disconnected and the robot lifted during the first hardware tests. Keep the USB recovery path available after OTA is working.

## Dashboard and local control

If Wi-Fi has not been configured, the board starts `Rocky-XIAO-Setup` and prints `DASHBOARD: http://192.168.4.1`. Save a 2.4 GHz network, reconnect to it, and open the exact `DASHBOARD: http://...` address printed by Serial Monitor. Never use `0.0.0.0` as the browser address.

The dashboard provides face and personality buttons, pan servo, individual motor tests and inversion, dance control, camera snapshot/live preview, microphone-to-speaker loopback, software speaker volume, wake-name test controls, MAX98357A tone testing, status, Wi-Fi configuration, a **What changed** release card, and **Restart and check OTA**. The machine-readable change log is also available at `/api/changes`. It is intentionally local and must not be port-forwarded to the public internet.

## Modularity rules

Add a personality under `modules/personalities/`. Add a face renderer under `modules/faces/`. Add bounded motion under `modules/motion/`. Add speaker playback under `modules/audio/`. Add speech adapters under `modules/voice/`. Add a board-specific pin map under `boards/`.

Each module must use the shared contracts in `modules/core/`, expose a deterministic test path, and avoid owning unrelated hardware or credentials. Read [`docs/MODULAR_ARCHITECTURE.md`](docs/MODULAR_ARCHITECTURE.md) before adding a feature.

The active face modules include Calm/procedural rendering, Rocky, Engineer, Spartan, and Isabella. Isabella and Spartan also retain dedicated state renderers. The visual direction is original and does not clone a real person’s voice or protected performance.

## Hardware references

The complete circuit and assembly artifacts are in [`hardware/wiring/`](hardware/wiring/): [`complete_wiring.png`](hardware/wiring/complete_wiring.png), [`assembly_layout.png`](hardware/wiring/assembly_layout.png), [`pin_tables.md`](hardware/wiring/pin_tables.md), [`compact_pin_tables.md`](hardware/wiring/compact_pin_tables.md), and [`assembly_layout.md`](hardware/wiring/assembly_layout.md).

The adjacent external signal allocation is:

| Function | XIAO pin |
|---|---|
| Servo signal | GPIO1 / D0 |
| MAX98357A DIN | GPIO2 / D1 |
| MAX98357A BCLK | GPIO3 / D2 |
| MAX98357A LRCK | GPIO4 / D3 |
| OLED SDA | GPIO5 / D4 |
| OLED SCL | GPIO6 / D5 |
| DRV8833 AIN1 | GPIO43 / D6 |
| DRV8833 AIN2 | GPIO44 / D7 |
| DRV8833 BIN1 | GPIO7 / D8 |
| DRV8833 BIN2 | GPIO8 / D9 |

The Sense camera and PDM microphone use their reserved internal pins. Do not reuse them.

## Local validation

Run from the Deskbot root:

```powershell
python tools/check_release.py
arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306" ESP32Servo
arduino-cli compile --fqbn "esp32:esp32:XIAO_ESP32S3:PSRAM=opi,FlashMode=qio,FlashSize=8M,USBMode=hwcdc,CDCOnBoot=default,UploadMode=default,PartitionScheme=default_8MB" firmware/xiao_esp32s3_sense
```

## Migration from the old local tree

Extract the flat ZIP directly over the existing `Deskbot` root. Do not create `Deskbot\rocky_walle_poc` or `Deskbot\Deskbot`. Use [`docs/MIGRATE_EXISTING_DESKBOT.md`](docs/MIGRATE_EXISTING_DESKBOT.md) to archive stale nested folders safely before the first clean commit.

## GitHub version and fleet OTA

`VERSION` is the human-maintained four-part OTA release source (`x.y.z.w`). Every release must include the complete firmware, all registered modules, the board profile, hardware documentation, workflow files, and an updated `CHANGE_SUMMARY.md`. Do not append the GitHub Actions run number.

GitHub Actions runs the release checker, compiles the XIAO board profile, creates the OTA-enabled release build, generates the manifest and SHA-256, and publishes GitHub Pages. Every board reports a unique ID such as `XIAO-ABCDEF012345` through `status` and `/api/status`.

To update all eligible boards:

```json
{"rollout":"all","boards":["all"]}
```

To update only selected boards:

```json
{"rollout":"selected","boards":["XIAO-ABCDEF012345"]}
```

Read [`docs/RELEASE_AND_FLEET_OTA.md`](docs/RELEASE_AND_FLEET_OTA.md) for enrollment, controlled rollout, recovery, and versioning.

## Change tracking

Every release updates [`CHANGE_SUMMARY.md`](CHANGE_SUMMARY.md) with the requirement, correction, changed files, and verification. The release checker rejects a missing module, invalid board profile, invalid fleet policy, local OTA target, or likely secret.

### References

[1]: https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html "Arduino ESP32 installation guide"
[2]: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/ "Seeed XIAO ESP32-S3 getting started guide"
[3]: https://wiki.seeedstudio.com/xiao_esp32s3_sense_mic/ "Seeed XIAO ESP32-S3 Sense microphone guide"
[4]: https://www.ti.com/product/DRV8833 "Texas Instruments DRV8833 product page"


For the exact v0.0.5.2 commit, push, OTA verification, microphone loopback, volume, and wake-name test checklist, read [`docs/RELEASE_V0.0.5.2.md`](docs/RELEASE_V0.0.5.2.md).

For v0.0.5.3 dual-Wi-Fi setup, remote OLED monitoring, version selection, and upgrade/downgrade testing, read [`docs/RELEASE_V0.0.5.3.md`](docs/RELEASE_V0.0.5.3.md).

For the complete version inventory, historical GitHub Release workflow, catalog refresh, and dashboard change-log verification, read [`docs/RELEASE_HISTORY.md`](docs/RELEASE_HISTORY.md).
