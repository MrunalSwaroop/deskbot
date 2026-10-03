# Deskbot Version History and Release Commands

## Important distinction

These are different objects:

- `VERSION` is the firmware version compiled by the current source tree.
- A Git commit saves source history.
- A Git tag names a specific commit.
- A GitHub Release is the downloadable firmware package.
- `ota/catalog.json` is the board’s selectable upgrade/downgrade list.

Folders such as `V.0.0.2` and `V0.0.3` are historical source folders, not OTA releases.

## Current v0.0.5.3 publish sequence

Run these commands from:

```powershell
C:\Users\msperavali\OneDrive - VE Commercial Vehicles Ltd\Mrunal\Deskbot
```

```powershell
Get-Content -LiteralPath ".\VERSION"
python tools\check_release.py
git diff --check
```

Expected:

```text
0.0.5.3
RELEASE CHECK OK: Deskbot 0.0.5.3; rollout=all; boards=['all']
```

Then commit and push:

```powershell
git status --short
git add .
git diff --cached --check
git commit -m "Deskbot v0.0.5.3 dual Wi-Fi remote OLED and release notes"
git push origin main
```

Wait for **Build all boards** and **Publish Deskbot OTA** to become green. The publish workflow creates:

- Git tag `v0.0.5.3`;
- GitHub Release `Deskbot v0.0.5.3`;
- `XIAO-ESP32S3.bin` release asset;
- Pages path `ota/releases/0.0.5.3/`;
- the current manifest;
- the retained `ota/catalog.json`.

Verify from PowerShell:

```powershell
Invoke-RestMethod "https://MrunalSwaroop.github.io/deskbot/ota/xiao-esp32s3-manifest.json"
Invoke-RestMethod "https://MrunalSwaroop.github.io/deskbot/ota/catalog.json"
gh release list --repo MrunalSwaroop/deskbot
```

The manifest should show `version : 0.0.5.3`. The catalog should contain the current release and any historical releases already created.

## Publish the historical releases

After v0.0.5.3 is pushed and the workflow file is visible on GitHub, start the one-time historical rebuild from PowerShell:

```powershell
gh workflow run publish-history.yml --repo MrunalSwaroop/deskbot --ref main
gh run list --repo MrunalSwaroop/deskbot --workflow publish-history.yml --limit 1
```

Wait until the four matrix jobs in **Publish historical Deskbot releases** are green. Then verify:

```powershell
gh release list --repo MrunalSwaroop/deskbot --limit 20
```

Expected official release tags include:

```text
v0.0.1
v0.0.4
v0.0.5.1
v0.0.5.2
v0.0.5.3
```

Finally rerun the current OTA workflow so it merges the GitHub Release assets into the Pages catalog:

```powershell
gh workflow run publish-ota.yml --repo MrunalSwaroop/deskbot --ref main
gh run list --repo MrunalSwaroop/deskbot --workflow publish-ota.yml --limit 1
```

After that run is green:

```powershell
$catalog = Invoke-RestMethod "https://MrunalSwaroop.github.io/deskbot/ota/catalog.json"
$catalog.versions | Select-Object version, firmware, current
```

The output should contain the five official versions. The board’s OTA selector will then be able to display those versions.

## Historical versions

The source history currently contains these meaningful XIAO milestones:

| Version | Source commit or tag | Meaning |
|---|---|---|
| 0.0.1 | `07578cf` | Frozen hardware baseline |
| 0.0.4 | tag `v0.0.4`, commit `9120a98` | OTA-stable core-3.3.7 baseline |
| 0.0.5.1 | `f95edb8` | OLED inversion and OTA feedback |
| 0.0.5.2 | `c314447` | Microphone loopback and voice boundary |
| 0.0.5.3 | current release commit after push | Dual Wi-Fi, remote OLED, retained catalog, and release notes |

`V.0.0.2` and `V0.0.3` are retained source folders and should not be represented as official OTA versions unless they are separately rebuilt and tested.

## Verify the dashboard release card

After the board updates to v0.0.5.3, open:

```text
http://XIAO_IP/
```

The top of the dashboard should show:

- **What changed**;
- release badge `v0.0.5.3`;
- a summary;
- a bullet list of the release changes;
- a link to `/api/changes`.

The direct endpoint is:

```text
http://XIAO_IP/api/changes
```

Expected JSON fields:

```json
{
  "version": "0.0.5.3",
  "title": "Dual Wi-Fi, remote OLED, and versioned OTA",
  "summary": "...",
  "changes": ["...", "..."]
}
```

## Selecting another OTA version

Use the dashboard catalog selector, or the serial command path:

```text
ota target 0.0.5.2
restart
```

To return to latest-release behavior:

```text
ota latest
restart
```

Only choose a version that has a valid `XIAO-ESP32S3.bin` asset in the catalog. Keep the robot stationary during installation.

## Safety checks before every release

```powershell
python tools\check_release.py
git diff --check
git status --short
```

Do not stage:

```text
ota_target.h
secrets.h
build\
*.bin
*.elf
*.map
```
