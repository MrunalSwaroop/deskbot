# Deskbot v0.0.5.1 — OLED Inversion and OTA Verification

This release is based on the verified **Deskbot v0.0.4** firmware. The remote Git tag `v0.0.4` points to the running OTA-stable commit.

## What changes in v0.0.5.1

- Adds persistent OLED inversion controls:
  - Serial: `oled invert on`, `oled invert off`, `oled invert toggle`
  - Dashboard: **Invert ON**, **Invert OFF**, and **Toggle**
  - The setting is stored in XIAO NVS and survives restart and OTA.
- Adds OTA state reporting to Serial, OLED, dashboard, and `/api/status`:
  - `checking`
  - `update_available`
  - `downloading` with percentage
  - `installing`
  - `rebooting`
  - `up_to_date`
  - `error`
- Changes GitHub release versioning to use the exact value in `VERSION`; this release is exactly `0.0.5.1` rather than a GitHub run-number suffix.

## Before committing

Run these commands from:

```powershell
C:\Users\msperavali\OneDrive - VE Commercial Vehicles Ltd\Mrunal\Deskbot
```

```powershell
git status --short --branch
git fetch origin
git diff --check
python tools\check_release.py
Select-String -Path VERSION -Pattern '^0\.0\.5\.1$'
Test-Path ".\firmware\xiao_esp32s3_sense\ota_target.h"
```

The last command must print `False`. Never commit `ota_target.h`, `secrets.h`, build output, or credentials.

## Commit and push

```powershell
git add .
git diff --cached --check
git commit -m "Deskbot v0.0.5.1: OLED inversion and OTA status"
git push origin main
```

If Git reports that the branch is not up to date, stop and run `git status --short --branch`. Do not use a force push for this release.

## Wait for GitHub Actions

Open **Actions** in the public `MrunalSwaroop/deskbot` repository.

1. **Build all boards** must finish with a green check.
2. **Publish Deskbot OTA** must finish with a green check.
3. Verify the manifest from PowerShell:

```powershell
Invoke-RestMethod `
  "https://MrunalSwaroop.github.io/deskbot/ota/xiao-esp32s3-manifest.json"
```

It must show:

```text
version : 0.0.5.1
rollout : all
```

The `source` commit must match the commit you pushed.

## Update the already-running board without USB

The board must already be running the OTA-capable v0.0.4 image and must be connected to the same Wi-Fi network as your phone/computer.

1. Open the dashboard at the exact `DASHBOARD: http://...` IP printed by the board.
2. Refresh the dashboard and confirm the old status shows firmware `0.0.4` and `otaConfigured: true`.
3. Leave motor power off during the update.
4. Press **Restart and check OTA** once.
5. Watch Serial Monitor at **115200 baud**. During this first transition, the old v0.0.4 image can only show its existing messages:

```text
OTA bootstrap: manifest version=0.0.5.1
OTA bootstrap: downloading XIAO firmware version 0.0.5.1
```

The new OLED progress screen and detailed `OTA state: ...` messages are part of v0.0.5.1 itself, so they become available after this first successful reboot. Do not press reset repeatedly during the download. The board reboots automatically after the image is accepted.

6. After reboot, wait for the normal Wi-Fi line and open the newly printed dashboard URL.
7. Press **Refresh** and confirm:

```text
firmware: 0.0.5.1
otaState: up_to_date
otaUpdateAvailable: false
```

## Verify the new OTA indication on later releases

After v0.0.5.1 is installed, a later release will show this sequence on Serial and the OLED:

```text
OTA state: checking
OTA state: update_available
OTA state: downloading 10%
OTA state: installing 100%
OTA state: rebooting 100%
```

The dashboard OTA banner and `/api/status` expose the same state.

## Verify OLED inversion

From Serial Monitor:

```text
oled invert on
oled invert off
oled invert toggle
status
```

Or use the dashboard’s **OLED display** section. Confirm that the OLED palette changes and that `OLED invert: on` or `OLED invert: off` appears in `status`. Restart once and confirm the selected palette remains active.

## If the board does not update

- `otaConfigured: false`: the board is still running a USB-only image; it needs the one-time OTA-capable bootstrap upload.
- `OTA state: up_to_date` with firmware `0.0.5.1`: the update already completed.
- `OTA state: error`: record the complete Serial error, verify Wi-Fi and the public manifest URL, then do not retry repeatedly.
- No dashboard: use the IP printed after Wi-Fi connects; do not use `0.0.0.0`.
- Boot loop after an image change: use the USB recovery path and keep the last known-good `v0.0.4` build available.

## Rollback rule

Do not delete or overwrite the `v0.0.4` tag. It is the recovery reference for the currently verified hardware state. A rollback is a recovery operation and should be done over USB unless a separately verified rollback image has been published.
