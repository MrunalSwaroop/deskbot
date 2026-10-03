# Deskbot Version History and Release Commands

## Current remote status at the start of v0.0.5.4

The cleaned `main` branch is synchronized with GitHub at the v0.0.5.3 source commit. At the time this release was prepared:

- GitHub Release published: `v0.0.5.3`
- GitHub tag present: `v0.0.5.3` and historical tag `v0.0.4`
- Historical source commits exist for `v0.0.1`, `v0.0.5.1`, and `v0.0.5.2`
- Historical GitHub Releases and OTA catalog entries still need the one-time `publish-history.yml` run
- The catalog therefore initially contains only `0.0.5.3`; after the historical workflow and a catalog refresh it should contain all five versions below

## What changed in every Deskbot version

| Version | Source milestone | Main changes | GitHub/OTA status |
|---|---|---|---|
| `0.0.1` | `07578cf` | Frozen XIAO ESP32-S3 Sense hardware baseline: OLED face, servo pan, DRV8833 dual motors, camera, onboard PDM microphone, MAX98357A audio path, Wi-Fi setup AP, local dashboard, personalities, face states, and modular project layout. | Historical source commit; publish as a Release using `publish-history.yml`. |
| `0.0.4` | `9120a983` / tag `v0.0.4` | OTA-stable build matched to ESP32 Arduino core `3.3.7`, verified XIAO Sense FQBN, correct 8 MB OTA partition, and recovery after the earlier OTA boot failure. | Tag exists; publish the binary as a historical Release. |
| `0.0.5.1` | `f95edb8` | Persistent OLED inversion (`on`, `off`, `toggle`), OTA state machine, update-available indication, download progress, installation/reboot states, and dashboard OTA status. | Source commit exists; publish the binary as a historical Release. |
| `0.0.5.2` | `c314447` | Onboard microphone monitoring, microphone-to-MAX98357A speaker loopback, software speaker volume, OLED microphone/loopback indicators, and wake-name engagement boundary without claiming speech recognition. | Source commit exists; publish the binary as a historical Release. |
| `0.0.5.3` | `5d226d0` | Two saved Wi-Fi profiles, profile failover, five-minute setup-AP fallback at `192.168.4.1`, remote OLED mirror, retained OTA catalog, intentional version target selection, release notes, and the improved change-log dashboard card. | Current published Release and OTA image. |
| `0.0.5.4` | next release | Phone-first responsive dashboard, touch-sized control grids, quick navigation, published-version history panel, and delayed OTA startup check. | Source prepared; publish after local validation. |

The folders `V.0.0.2` and `V0.0.3` are historical source folders, not official OTA versions.

## Publish v0.0.5.4

From the Windows Deskbot root:

```powershell
cd -LiteralPath "C:\Users\msperavali\OneDrive - VE Commercial Vehicles Ltd\Mrunal\Deskbot"
Get-Content -LiteralPath ".\VERSION"
python tools\check_release.py
git diff --check
git status --short
```

Expected version/check output:

```text
0.0.5.4
RELEASE CHECK OK: Deskbot 0.0.5.4; rollout=all; boards=['all']
```

Then commit and push:

```powershell
git add .
git diff --cached --check
git commit -m "Deskbot v0.0.5.4 mobile dashboard and deferred OTA"
git push origin main
```

Wait for **Build all boards**, **Deskbot release check**, and **Publish Deskbot OTA** to become green.

## Publish the historical releases once

After the v0.0.5.4 workflow is green:

```powershell
gh workflow run publish-history.yml --repo MrunalSwaroop/deskbot --ref main
$historyRunId = gh run list --repo MrunalSwaroop/deskbot --workflow publish-history.yml --limit 1 --json databaseId --jq '.[0].databaseId'
gh run watch $historyRunId --repo MrunalSwaroop/deskbot --exit-status
```

Then verify:

```powershell
gh release list --repo MrunalSwaroop/deskbot --limit 20
```

Expected official releases:

```text
v0.0.1
v0.0.4
v0.0.5.1
v0.0.5.2
v0.0.5.3
v0.0.5.4
```

## Refresh the catalog

After historical publishing succeeds, run the current OTA workflow again:

```powershell
gh workflow run publish-ota.yml --repo MrunalSwaroop/deskbot --ref main
$catalogRunId = gh run list --repo MrunalSwaroop/deskbot --workflow publish-ota.yml --limit 1 --json databaseId --jq '.[0].databaseId'
gh run watch $catalogRunId --repo MrunalSwaroop/deskbot --exit-status
$catalog = Invoke-RestMethod "https://MrunalSwaroop.github.io/deskbot/ota/catalog.json"
$catalog.versions | Sort-Object version | Format-Table version, current, firmware
```

Expected versions:

```text
0.0.1
0.0.4
0.0.5.1
0.0.5.2
0.0.5.3
0.0.5.4
```

## Phone dashboard verification

Open the exact IP printed by the board, for example:

```text
http://192.168.31.139/
```

On a phone, verify:

- quick-navigation pills for Status, Faces, Motion, Audio, Camera, and OTA;
- controls arranged in touch-sized grids rather than long inline button rows;
- status and OLED mirror cards fit the screen without horizontal scrolling;
- the Firmware and OTA card shows the published version history;
- **What changed** shows the installed release and its change list;
- the dashboard remains reachable during the first seconds of boot before the OTA check begins.

## Reboot and OTA timing

The previous firmware called the HTTPS manifest check from `setup()`. This meant TLS, manifest parsing, and a firmware download could delay the first usable dashboard. v0.0.5.4 moves the first OTA check out of the critical boot path:

1. hardware initializes;
2. Wi-Fi and `WebServer` start;
3. the dashboard becomes reachable;
4. four seconds later the OTA manifest check begins;
5. if an update is available, the download temporarily pauses normal firmware work and then reboots safely.

A real OTA download cannot safely run completely invisibly while the firmware is driving motors. Keep the robot stationary and motor power off during installation.

## Safety checks before every release

```powershell
python tools\check_release.py
git diff --check
git status --short
```

Never stage:

```text
ota_target.h
secrets.h
build\
*.bin
*.elf
*.map
```
