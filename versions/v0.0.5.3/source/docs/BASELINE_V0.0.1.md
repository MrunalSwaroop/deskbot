# Deskbot v0.0.1 Baseline

## Purpose

`v0.0.1` is the frozen hardware bring-up baseline for the Seeed XIAO ESP32-S3 Sense Deskbot. Future feature work should branch from this state and use higher semantic versions.

## Confirmed baseline hardware

- XIAO ESP32-S3 Sense boots the clean-root firmware.
- OLED is initialized and displays the Deskbot face.
- Sense camera is detected after reseating the Sense expansion board.
- Onboard PDM microphone initializes at 16 kHz.
- MAX98357A I2S output initializes at 16 kHz.
- Pan servo, DRV8833 motor driver, and two N90 motors remain part of the hardware contract.
- Local dashboard is available through the printed device IP or setup AP.
- Wi-Fi is provisioned through `Rocky-XIAO-Setup` and the local `/wifi` page.

## Known baseline limitations

Expression artwork and state mapping are intentionally **not considered final** in v0.0.1. Known expression tuning work is deferred to the next release so this version remains a reproducible hardware baseline. The expression modules remain replaceable under `modules/faces/`.

The microphone currently reports a smoothed level meter; it does not perform speech-to-text. The MAX98357A path currently provides the isolated tone test; full voice playback is a later module release.

## One-time OTA bootstrap

The current local USB build has OTA disabled by design. A board cannot turn OTA on from a firmware image that does not contain the OTA service configuration. Therefore:

1. Use the current known-good USB connection **one final time**.
2. In the firmware folder, copy:

   ```powershell
   Copy-Item ota_target.example.h ota_target.h
   ```

3. Upload the resulting v0.0.1 OTA bootstrap with the OTA-capable `Default with spiffs` partition scheme, `OPI PSRAM`, and `XIAO_ESP32S3` selected.
4. Configure Wi-Fi through `Rocky-XIAO-Setup` if it is not already stored.
5. Confirm the board reports its IP and requests the GitHub Pages manifest.
6. Remove the local `ota_target.h` after uploading; it must never be committed.

After this one-time bootstrap, future releases are remote-only:

1. Update modules and `VERSION` in the Deskbot repository.
2. Run the release checker and compile locally.
3. Push `main` to `MrunalSwaroop/deskbot`.
4. GitHub Actions publishes the newer image and manifest to GitHub Pages.
5. Open the dashboard and press **Restart and check OTA**. The board checks the manifest at boot and downloads only a newer version for the selected fleet policy.

Keep USB available as a recovery path, but do not connect it for normal updates.

## v0.0.1 acceptance check

Before treating the baseline as frozen, verify:

```text
status
test oled
test camera
test mic
test audio
test servo
personality calm
personality rocky
personality spartan
personality isabella
```

Test motors lifted and with external motor power controlled by the physical switch.
