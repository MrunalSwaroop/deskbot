# Deskbot v0.0.5.4 — Phone Dashboard and Deferred OTA

This release is based on the cleaned v0.0.5.3 source. It improves phone usability and moves the first OTA check out of the critical boot path.

## Changes

- Responsive mobile dashboard with touch-sized controls and quick navigation.
- Version-history panel sourced from the OTA catalog.
- Current release change card remains available through `/api/changes`.
- OTA manifest checking begins four seconds after the dashboard startup path.
- OTA downloads still reboot safely and must be performed with motors stopped.

## Windows release steps

```powershell
cd -LiteralPath "C:\\Users\\msperavali\\OneDrive - VE Commercial Vehicles Ltd\\Mrunal\\Deskbot"
Get-Content -LiteralPath ".\\VERSION"
python tools\\check_release.py
git diff --check
git status --short
git add .
git diff --cached --check
git commit -m "Deskbot v0.0.5.4 mobile dashboard and deferred OTA"
git push origin main
```

Expected check output:

```text
0.0.5.4
RELEASE CHECK OK: Deskbot 0.0.5.4; rollout=all; boards=['all']
```

Wait for the three workflows to become green:

- `Deskbot release check`
- `Build all boards`
- `Publish Deskbot OTA`

## Board verification

1. Keep motor power off.
2. Open the board IP printed by Serial Monitor on a phone.
3. Confirm the dashboard appears before the OTA check starts.
4. Confirm the Firmware and OTA card shows `0.0.5.4`.
5. Confirm the **What changed** card lists the phone dashboard and delayed OTA changes.
6. Confirm the version-history card lists every catalog entry available at that time.
7. Test one face, one personality, one pan command, and the OLED mirror.
8. Use **Restart and check OTA** only with the robot stationary.

## Why OTA is not completely invisible

The manifest check is now delayed until after the dashboard starts. If a firmware image must be downloaded, the ESP32 HTTP update operation temporarily occupies the firmware loop so it can write the inactive OTA partition safely. A fully concurrent motor-driving OTA path would be unsafe; the correct policy is to keep the robot stationary during installation.

## References

- [ESP32 OTA update documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/ota_updates.html)
- [GitHub Actions](https://docs.github.com/en/actions)
- [GitHub Releases](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases)
