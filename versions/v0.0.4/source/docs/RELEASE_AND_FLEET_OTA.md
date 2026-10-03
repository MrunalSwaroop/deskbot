# Deskbot Releases and Fleet OTA

The Deskbot root is the source of truth. The normal cycle is: change modules, run tests, update `VERSION` when the release behavior changes, update `CHANGE_SUMMARY.md`, commit and push `main`, wait for GitHub Actions, then roll out to one selected board before expanding the fleet.

The frozen starting point is **v0.0.1**. Expression tuning is deliberately deferred to a later version so this release remains the reproducible hardware baseline.

Use this Windows root:

```powershell
cd -LiteralPath "C:\Users\msperavali\OneDrive - VE Commercial Vehicles Ltd\Mrunal\Deskbot"
```

Do not create a nested repository.

## Versioning

Use semantic versions. A patch is a correction, a minor version is a backward-compatible feature, and a major version changes pins, protocol, or storage. The workflow adds its run number to the patch component so every published image is newer.

## Board enrollment

Each board prints an ID through `status` and exposes it at `/api/status`:

```json
{
  "boardId": "XIAO-ABCDEF012345",
  "firmware": "0.0.1-base"
}
```

Edit `fleet/selected_boards.json` before a controlled release.

```json
{"rollout":"all","boards":["all"]}
```

or:

```json
{"rollout":"selected","boards":["XIAO-ABCDEF012345"]}
```

## One-time USB bootstrap

Each board needs one final USB-uploaded OTA-capable v0.0.1 image with the correct XIAO target, `OPI PSRAM`, OTA partition scheme, and local Wi-Fi configured through the setup AP. Copy `ota_target.example.h` to the ignored local `ota_target.h`, upload once, then delete the local copy. After that bootstrap, normal updates do not require USB; GitHub Actions publishes the newer image and the board downloads it after reboot. Keep the USB cable and Arduino IDE available for recovery. Do not use OTA while the robot is moving or motor power is on.

## GitHub workflows

`build-all-boards.yml` compiles the board profiles without private files. `publish-ota.yml` creates the OTA-enabled XIAO image, writes the HTTPS manifest, computes SHA-256, and publishes GitHub Pages. `release-check.yml` validates the repository contract independently.

GitHub Pages must use **GitHub Actions** as its source. For the free unauthenticated device path, the repository and Pages site must be public. The local dashboard remains private on the local network.

## Safe rollout

1. Set the fleet policy to one pilot board.
2. Push the release.
3. Confirm the pilot reports the new firmware version.
4. Verify OLED, dashboard, camera, microphone, audio, and motor safety stop.
5. Add remaining board IDs or switch the policy to `all`.
6. Reboot boards while stationary and verify each local dashboard.

The firmware checks the manifest once after Wi-Fi becomes available. An unselected board remains on its current version.

## Recovery

If an OTA image fails, use BOOT plus RESET and upload the last known-good or corrected firmware over USB. Publish a higher version containing the correction; do not reuse an older version number.

Never commit `ota_target.h`, Wi-Fi passwords, API keys, `.env` files, build folders, compiled binaries, or serial logs containing secrets.

### References

[1]: https://docs.github.com/en/actions "GitHub Actions documentation"
[2]: https://docs.github.com/en/pages "GitHub Pages documentation"
[3]: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/ "Seeed Studio XIAO ESP32-S3 getting started guide"
