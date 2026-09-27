# XIAO next step: OTA, dashboard visibility, and reusable desk-pet behavior

The current XIAO firmware is now the main hardware base. It already drives the OLED, pan servo, DRV8833 tracks, Sense camera, PDM microphone level monitor, Wi-Fi setup page, local dashboard, head-state behavior, dance routine, and Rocky/Isabella personality profiles. The next addition is **safe remote firmware delivery**. The board will still receive the first OTA-capable image over USB, and it will only install a newer image after comparing a small HTTPS manifest.

This approach reuses the verified manifest/update pattern from the referenced `DB_PetBot` project.[1] It does not require Xiaozhi.me. It also keeps the visual behavior from the desktop-pet OLED reference modular: the hardware sketch owns the face renderer, while personality-specific behavior remains replaceable rather than being mixed into the network update layer.

## What was added

The XIAO sketch now includes `device_config.h`, `ota_service.h`, `ota_service.cpp`, and the GitHub Pages root certificate. The OTA service reuses the existing `rocky-xiao` non-volatile storage namespace and the same `ssid`/`pass` keys used by the dashboard Wi-Fi page. This means OTA does not create a second provisioning system.

The dashboard status response now reports the running `firmware` version and whether an OTA `manifest` is configured. The dashboard explains that updates are checked once during boot. A failed update check does not stop the OLED, camera, microphone, servo, motors, or local dashboard.

Two GitHub Actions workflows are included. The build workflow compiles the current XIAO sketch on every relevant push or pull request. The Pages workflow creates a versioned XIAO binary, publishes `xiao-esp32s3-manifest.json`, and deploys the files under GitHub Pages. The workflow does not publish Wi-Fi passwords, API keys, or `ota_target.h`.

## First USB bootstrap

Before using OTA, compile and upload the current sketch once over USB. Keep `ota_target.h` absent during this first compile if you only want to verify the hardware. The firmware then uses the existing dashboard flow to connect to Wi-Fi:

1. Upload `xiao_bringup/xiao_bringup.ino` with Arduino IDE or Arduino CLI.
2. If no Wi-Fi credentials are stored, connect to `Rocky-XIAO-Setup` and open `http://192.168.4.1`.
3. Save a 2.4 GHz Wi-Fi network from the **Configure Wi-Fi** page.
4. Reconnect the computer or phone to that same network and open the XIAO IP shown in Serial Monitor.
5. Confirm that `/api/status` shows the current firmware version and `otaConfigured: false`.

The first USB upload is the recovery path. If a later OTA image is bad, use BOOT mode and USB upload to restore a known-good image.

## Connect the project to GitHub Pages

The publishing workflow expects this project to be pushed to a GitHub repository. GitHub Pages must be enabled with **GitHub Actions** as its source. The workflow derives the Pages URL from the repository owner and repository name, so the firmware does not need a hard-coded personal URL in the source tree.

The workflow publishes these files:

```text
https://<owner>.github.io/<repository>/ota/XIAO-ESP32S3.bin
https://<owner>.github.io/<repository>/ota/xiao-esp32s3-manifest.json
```

Create the local target file only when you are ready to enable OTA:

```text
xiao_bringup/ota_target.example.h  ->  xiao_bringup/ota_target.h
```

The ignored `ota_target.h` must point to the Pages URLs for the repository. The example file contains `YOUR_OWNER` and `YOUR_REPOSITORY` placeholders that must be replaced. Do not commit the resulting file if it contains a private development endpoint. The public Pages URL itself is not a secret, but the file is ignored to keep release configuration local and explicit.

Upload this manifest-enabled image over USB once. After that, a newer GitHub Actions run can be installed by the board at boot. The current USB bootstrap is `0.3.0`; the Pages workflow publishes `0.3.<run-number>`, so the first successful Pages build is newer. The service compares semantic versions and does not reinstall an equal or older version.

## Expected OTA serial output

A healthy device should show a sequence similar to the following after the normal Wi-Fi connection:

```text
OTA bootstrap: starting XIAO ESP32-S3 Wi-Fi service
OTA bootstrap: using Wi-Fi credentials stored on board
OTA bootstrap: using active Wi-Fi connection, IP=<xiao-ip>
OTA bootstrap: requesting XIAO HTTPS manifest
OTA bootstrap: manifest request result=200
OTA bootstrap: manifest version=0.3.1
OTA bootstrap: already running version 0.3.1
```

When a newer image exists, the final lines change to:

```text
OTA bootstrap: manifest version=0.3.2
OTA bootstrap: downloading XIAO firmware version 0.3.2
OTA bootstrap: XIAO update accepted; board will reboot
```

The update path uses HTTPS, a pinned GitHub Pages root certificate, the ESP32 OTA partition mechanism, and a long stream timeout for slower networks. In Arduino IDE, select a partition scheme that provides OTA slots. Do not select a maximum-application scheme that removes OTA partitions.

## Dashboard checks after the update

Open the dashboard and verify the following before connecting motor power:

| Dashboard item | Expected result |
|---|---|
| Status | `firmware` shows the compiled version and `otaConfigured` shows whether the target header is present |
| Camera | Snapshot and low-rate live mode remain available |
| Microphone | `test mic` changes the reported level when you speak or clap near the Sense microphone |
| Head and dance | State-following and dance continue to operate after OTA service startup |
| Motors | `test left` and `test right` still work with the robot lifted and motor power isolated |
| Wi-Fi | The dashboard remains on the same local IP after reboot unless the access point assigns a new lease |

The update check happens once per boot. This is intentional: it avoids repeated downloads, keeps the control loop responsive, and makes a failed cloud check non-fatal.

## How the referenced desktop-pet OLED work fits

The desktop-pet task established a useful separation between **character identity**, **state animation**, and **hardware control**. The current XIAO sketch follows the same boundary even though its character art is currently procedural:

- The `Personality` enum selects Calm, Rocky, Engineer, Spartan, or Isabella.
- `FaceMode` selects idle, listening, thinking, working, speaking, happy, laugh, curious, sad, surprised, error, or sleep.
- `drawEyes`, `drawPersonalityAccent`, and `drawMouth` render the current combination.
- Servo poses and motor choreography respond to state changes but are not embedded in the OLED drawing functions.
- The dashboard and Serial Monitor send the same command strings.

This is the correct place to add the richer Isabella state bitmap set and the recognizable Rocky/Spartan character frames from the referenced OLED work later. They should be added as optional headers or frame tables, not copied into the OTA service. The board can then update character assets and behavior through the same firmware release mechanism.

## What OTA does not do yet

OTA updates firmware. It does not update the camera model, add a cloud LLM, provide text-to-speech, or make the local dashboard public. The next voice stage still needs a speaker path, audio playback test, push-to-talk or wake-word policy, and a backend choice. The microphone test currently measures signal level only.

The dashboard is intentionally unauthenticated and local. Do not port-forward it to the internet. If remote administration is needed later, put an authenticated relay or managed dashboard in front of a device-specific command channel rather than exposing the ESP32 WebServer directly.

## Recommended next implementation order

First, prove that the current firmware still compiles and runs after adding the OTA module. Second, publish a test manifest with the same version and verify that the board refuses to reinstall it. Third, publish a deliberately small version increment and verify one controlled update while the robot is stationary. Fourth, restore the richer character-frame headers and test them on the OLED without motors. Only after USB recovery and one successful OTA update are proven should the project add continuous voice and camera-presence automation.

## References

[1]: https://github.com/MrunalSwaroop/DB_PetBot "Referenced DB_PetBot XIAO OTA and GitHub Actions implementation"
[2]: https://github.com/TechTalkies/Xiaozhi-for-XiaoESP32S3 "Referenced Xiaozhi-for-XiaoESP32S3 hardware and voice project"
[3]: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/ "Seeed Studio XIAO ESP32-S3 getting started guide"
[4]: https://wiki.seeedstudio.com/xiao_esp32s3_sense_mic/ "Seeed Studio XIAO ESP32-S3 Sense microphone guide"
