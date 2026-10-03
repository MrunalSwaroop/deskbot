# Deskbot XIAO ESP32-S3 Sense Firmware

This is the active firmware target for Deskbot. The integration sketch coordinates stable modules for the OLED, pan servo, DRV8833 two-channel motor driver, camera, PDM microphone, MAX98357A amplifier, local dashboard, Wi-Fi provisioning, and OTA.

The sketch is intentionally a bring-up base. It proves each hardware function independently before adding speech recognition, text-to-speech, or a cloud model adapter.

## Source layout

| Path | Purpose |
|---|---|
| `xiao_esp32s3_sense.ino` | Hardware integration and local dashboard |
| `../../boards/xiao-esp32s3-sense/pinmap.h` | Active GPIO map and compile-time conflict checks |
| `device_config.h` | Build defaults and OTA feature switch |
| `ota_service.cpp/.h` | HTTPS manifest, board identity, semantic versioning, and fleet selection |
| `../../modules/core/` | Shared face-state and personality contracts |
| `../../modules/faces/` | Isabella, Spartan, and Rocky OLED renderers and frames |
| `../../modules/personalities/` | Replaceable personality descriptions and registry entries |
| `../../modules/motion/` | Servo, track, and dance contracts |
| `../../modules/audio/` | MAX98357A and tone-test contracts |
| `../../modules/voice/` | Local voice backend contracts; no cloud dependency is required |
| `../../modules/vision/` | Sense camera contract |
| `../../modules/sensing/` | Sense PDM microphone contract |
| `../../modules/network/` | Wi-Fi and local dashboard contracts |
| `../../modules/ota/` | Fleet manifest contract |

The integration sketch remains the only place that wires the verified hardware functions together. New features should be added through the module contracts instead of copying GPIO logic into personality files.

## Arduino IDE preparation

Install the Espressif ESP32 board package from the official package index [1]. Install these libraries through Arduino IDE’s Library Manager:

- Adafruit GFX Library
- Adafruit SSD1306
- ESP32Servo

Open this sketch:

```text
firmware/xiao_esp32s3_sense/xiao_esp32s3_sense.ino
```

Select these Arduino IDE settings:

| Setting | Value |
|---|---|
| Board | `XIAO_ESP32S3` |
| Flash Size | `8MB (64Mb)` |
| PSRAM | `OPI PSRAM` |
| Partition Scheme | An OTA-capable `Default with spiffs` scheme |
| Upload Mode | `UART0 / Hardware CDC` |
| USB Mode | `Hardware CDC and JTAG` |
| Upload Speed | `921600` or a lower stable value |
| Port | Detected XIAO USB port |
| Serial Monitor | `115200 baud`, newline enabled |

The Sense camera requires PSRAM. The OTA service requires an OTA-capable partition scheme. Do not choose a maximum-application partition that removes the alternate OTA slot.

## First USB flash

The v0.0.1 baseline is intentionally verified over USB. To move to remote-only updates, make the next upload the one-time OTA-capable bootstrap; later feature releases can be installed through GitHub without reconnecting USB.

1. Disconnect motor power and keep the robot lifted.
2. Connect the XIAO with a data-capable USB-C cable.
3. Select `XIAO_ESP32S3` and the settings above.
4. For the final bootstrap only, copy `ota_target.example.h` to the ignored local file `ota_target.h`. This enables OTA and points the board at the public Deskbot Pages manifest. Do not commit it.
5. Compile and upload the sketch.
6. Open Serial Monitor at `115200`.
7. If the board does not appear, hold BOOT, tap RESET, release BOOT, then select the bootloader port.
8. Confirm the boot log reaches `Deskbot XIAO modular bring-up ready` and reports the GitHub Pages manifest request.

After confirming the bootstrap, delete the local `ota_target.h`. The board retains the OTA-enabled firmware internally; future releases are uploaded by pushing GitHub `main` and rebooting the board. A normal local USB build prints that OTA is disabled. No Wi-Fi password or API key belongs in the repository.

After a future GitHub release finishes, use the dashboard button **Restart and check OTA** or send the Serial-equivalent command `restart`. No USB connection is needed for this normal update cycle.

## Wi-Fi and dashboard

If no credentials are stored, the board starts a setup access point:

```text
SETUP AP: Rocky-XIAO-Setup IP: 192.168.4.1
DASHBOARD: http://192.168.4.1
```

Connect a phone or laptop to `Rocky-XIAO-Setup`, open `http://192.168.4.1`, choose **Configure Wi-Fi**, enter a 2.4 GHz network, and save. Reconnect to that network and open the IP printed by Serial Monitor. If the AP does not appear, press the board reset button once, wait for the `SETUP AP` line, and connect to the AP before opening the page.

For a direct first-boot connection instead, copy `secrets.h.example` to the same firmware folder as `secrets.h`, replace the two placeholders, upload once over USB, and keep `secrets.h` local. The file is ignored and must not be committed.

The local dashboard is available at:

```text
http://XIAO_IP/
http://XIAO_IP/health
http://XIAO_IP/api/status
http://XIAO_IP/wifi
```

It provides face and personality buttons, servo control, motor tests and inversion, dance control, camera snapshot/live preview, microphone monitoring, MAX98357A tone testing, status, and Wi-Fi configuration. It is intentionally local and must not be port-forwarded to the public internet.

If the normal Wi-Fi connection succeeds but no page opens, use the exact `DASHBOARD: http://...` address printed by the board and make sure the phone/computer is on the same 2.4 GHz network. Do not use `0.0.0.0`; it is a bind/listen address, not a browser destination.

The status API reports a unique board identity and OTA state such as:

```json
{
  "boardId": "XIAO-ABCDEF012345",
  "firmware": "0.0.5.2",
  "otaConfigured": true,
  "otaState": "up_to_date",
  "otaProgress": 100,
  "otaUpdateAvailable": false
}
```

Record each board ID in a private fleet inventory. Do not put credentials in the board registry.

## Hardware test order

Run tests in this order so one fault does not hide another:

```text
test oled
face idle
face listening
face thinking
face working
face speaking
```

Then test the servo with the linkage disconnected from hard stops:

```text
test servo
pan 90
pan 60
pan 120
```

Use `pan 0` and `pan 180` only after confirming the physical mechanism can reach those angles safely.

Test the motor channels with the robot lifted and VM isolated:

```text
motor speed 80
test left
test right
motor stop
```

After both channels work independently, test short combined commands:

```text
motor forward
motor stop
motor back
motor stop
motor left
motor stop
motor right
motor stop
```

The DRV8833 `nSLEEP/SLP` input must be tied HIGH to 3V3. `VM` uses the separate motor supply. Motor and servo supplies must share ground with the XIAO. Use the runtime inversion commands instead of recompiling for a polarity correction:

```text
motor invert left on
motor invert left off
motor invert right on
motor invert right off
```

The firmware applies a motor timeout safety stop.

## Audio and microphone tests

The MAX98357A speaker must connect between `SPK+` and `SPK−`. Do not connect either speaker lead to ground. Use the hardware table in `hardware/wiring/pin_tables.md` and the complete diagram in `hardware/wiring/complete_wiring.png`.

Run the audio test only after confirming power and speaker wiring:

```text
test audio
audio tone 440 1500
audio stop
```

Run the onboard Sense microphone test separately:

```text
test mic
mic monitor
mic off
```

The microphone test reports level changes only. It is not speech-to-text. Voice integration will be added behind `modules/voice/` after the electrical microphone and speaker tests are stable.

## Camera

The Sense camera is connected through the Sense board connector. Enable `OPI PSRAM` and do not reuse its reserved GPIOs. Run:

```text
test camera
```

Then open:

```text
http://XIAO_IP/camera.jpg
http://XIAO_IP/camera/live
```

The live page uses repeated JPEG snapshots instead of a blocking stream so the dashboard, OLED, servo, motors, and microphone remain responsive.

If the Serial Monitor says `CAMERA ERROR`, re-check `OPI PSRAM`, the Sense camera connector, and that the sketch is the active clean-root file `firmware/xiao_esp32s3_sense/xiao_esp32s3_sense.ino`.

## Personalities and faces

The stable commands are:

```text
personality calm
personality rocky
personality engineer
personality spartan
personality isabella
```

The display modules also support their own state commands:

```text
isabella neutral
isabella shy
isabella happy
isabella concerned
isabella listening
isabella speaking
isabella curious
isabella thinking
isabella sad
isabella laughing
isabella surprised
isabella sleeping

spartan guard
spartan command
spartan salute
spartan listening
spartan thinking
spartan laughing
spartan march
spartan attack
spartan victory
spartan sleeping
```

Rocky uses the supplied modular body-language renderer rather than the old eye-only face. Test it independently:

```text
personality rocky
rocky idle
rocky curious
rocky greeting
rocky listening
rocky forward
rocky back
rocky pace
rocky come
rocky go
rocky wake
rocky dance
```

`face speaking` now maps Rocky to a greeting/body-language pose. For Isabella, `face speaking` or `isabella speaking` animates the mouth and gesture cues on successive frames; it is not a static bitmap. `face sad`, `face curious`, and `face thinking` map to different Isabella overlays.

To add a personality, add a registry entry and one module under `modules/personalities/`, optionally add a face renderer under `modules/faces/`, add a test command, compile, and update `CHANGE_SUMMARY.md`. Removing the registry entry and its files does not change the motor, camera, microphone, audio, or OTA interfaces.

## GitHub release and OTA

The root `VERSION` file is the human-maintained release source. Push changes from the Deskbot root. GitHub Actions runs the release checker, compiles the XIAO board profile, and publishes a versioned binary and manifest to GitHub Pages.

Before OTA, each board must have one USB-uploaded OTA-capable baseline. The release workflow creates its own ignored `ota_target.h` during CI; it is not committed.

Fleet selection is controlled by `fleet/selected_boards.json`:

```json
{
  "rollout": "all",
  "boards": ["all"]
}
```

or:

```json
{
  "rollout": "selected",
  "boards": ["XIAO-ABCDEF012345"]
}
```

A board updates only when the device type matches, the semantic version is newer, its board ID is selected, and the OTA image is downloaded through the configured HTTPS manifest. Reboot boards while stationary and verify their dashboards after the update.

Keep USB recovery available. If a release misbehaves, publish a higher version containing the correction instead of reusing an older version number.

### References

[1]: https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html "Arduino ESP32 installation guide"
[2]: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/ "Seeed XIAO ESP32-S3 getting started guide"
[3]: https://wiki.seeedstudio.com/xiao_esp32s3_sense_mic/ "Seeed XIAO ESP32-S3 Sense microphone guide"
[4]: https://www.ti.com/product/DRV8833 "Texas Instruments DRV8833 product page"


## OLED inversion and OTA feedback

The OLED palette can be changed without reflashing:

```text
oled invert on
oled invert off
oled invert toggle
```

The dashboard exposes the same controls. The OTA-capable image reports `checking`, `update_available`, `downloading`, `installing`, `rebooting`, `up_to_date`, or `error` through Serial, the OLED progress screen, the dashboard banner, and `/api/status`.

Follow [`../../docs/RELEASE_V0.0.5.2.md`](../../docs/RELEASE_V0.0.5.2.md) for the exact commit, push, cable-free update, microphone loopback, volume, and wake-name test sequence.


## v0.0.5.2 microphone and speaker test

This release has two separate microphone modes:

- `mic monitor` — measures the onboard PDM microphone and displays a smoothed level; it does not play audio.
- `mic loopback 60` — routes microphone PCM to the MAX98357A for 60 seconds. Start with the software volume at 20%.

Commands:

```text
audio volume 20
test mic
mic loopback 60
audio stop
mic off
status
```

The dashboard has the same controls. The OLED shows a small microphone meter while monitoring and a loopback marker while audio is being routed. Volume is software scaling, not a replacement for a physical amplifier gain control. Keep the speaker connected only between `SPK+` and `SPK-`; never connect either speaker output to ground.

### Wake-name boundary

`wake name rocky`, `wake on`, `wake off`, and `wake simulate` test the engagement path. The v0.0.5.2 loopback path cannot identify spoken words by itself; a raw amplitude meter can tell that sound exists but cannot tell whether the word was “Rocky”. A later local wake-word engine or Xiaozhi/relay adapter must call the same engagement function after actual recognition.
