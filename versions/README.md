# Deskbot version folders

These are readable source snapshots for the six published XIAO ESP32-S3 Sense milestones. Each snapshot contains the active modular source, board profile, hardware documentation, workflows, and release documentation. Generated binaries and credentials are not stored in Git.

| Folder | Source commit | Main purpose | GitHub Release |
|---|---|---|---|
| [`v0.0.1/`](v0.0.1) | `07578cf026eb161585037c67768f1b4a59ac9450` | Frozen XIAO ESP32-S3 Sense hardware baseline | [Release](https://github.com/MrunalSwaroop/deskbot/releases/tag/v0.0.1) |
| [`v0.0.4/`](v0.0.4) | `9120a983b3ed8a17bfb8108c6694ef7dbb7d3194` | OTA-stable ESP32 Arduino core 3.3.7 baseline | [Release](https://github.com/MrunalSwaroop/deskbot/releases/tag/v0.0.4) |
| [`v0.0.5.1/`](v0.0.5.1) | `f95edb852325a56a45b004a5ab532f8c2b33f6a1` | OLED inversion and OTA state/progress UI | [Release](https://github.com/MrunalSwaroop/deskbot/releases/tag/v0.0.5.1) |
| [`v0.0.5.2/`](v0.0.5.2) | `c314447c3eacd343f680d7743e713bf62445ca13` | Microphone loopback, speaker volume, and wake-name boundary | [Release](https://github.com/MrunalSwaroop/deskbot/releases/tag/v0.0.5.2) |
| [`v0.0.5.3/`](v0.0.5.3) | `5d226d0db5bbb8b0b62f9164e7e145dcdf0be262` | Dual Wi-Fi, setup fallback, remote OLED, and versioned OTA catalog | [Release](https://github.com/MrunalSwaroop/deskbot/releases/tag/v0.0.5.3) |
| [`v0.0.5.4/`](v0.0.5.4) | `522c56ab17a76cb1aa1095f2c555ba99d42b39a4` | Phone-first dashboard and deferred OTA startup check | [Release](https://github.com/MrunalSwaroop/deskbot/releases/tag/v0.0.5.4) |

## Build a snapshot

From inside any `versions/vX.Y.Z/source/` directory, use the active XIAO sketch path:

```text
arduino-cli compile --fqbn XIAO_ESP32S3_FQBN firmware/xiao_esp32s3_sense
```

Use each snapshot’s board profile and release documentation for exact settings and OTA safety.
