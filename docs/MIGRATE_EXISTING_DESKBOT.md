# Migrate an Existing Deskbot Folder

The modular bundle is flat: extract it directly into the existing `Deskbot` folder, not into `Deskbot\rocky_walle_poc` and not into `Deskbot\Deskbot`.

Before cleanup, make a copy of the whole `Deskbot` folder if you want an additional manual backup. Then run this one-time PowerShell block from the existing repository root. It moves only stale historical top-level folders/files into a timestamped sibling backup; it does not delete them.

```powershell
$repo = (Get-Location).Path
$backup = Join-Path (Split-Path $repo -Parent) ("Deskbot-before-modular-" + (Get-Date -Format "yyyyMMdd-HHmmss"))
New-Item -ItemType Directory -Path $backup -Force | Out-Null
$old = @(
  "rocky_walle_poc",
  "xiao_bringup",
  "esp32_gateway",
  "eyes_only_test",
  "integration_relay",
  "llm_bridge",
  "uno_r4_current_desk_buddy",
  "uno_r4_face_motion",
  "uno_wifi_face_server",
  "face_neck_test.ino",
  "eyes.h"
)
foreach ($name in $old) {
  $source = Join-Path $repo $name
  if (Test-Path -LiteralPath $source) {
    Move-Item -LiteralPath $source -Destination $backup -Force
  }
}
Write-Host "Historical files archived at $backup"
```

After the move, the active repository root should contain `boards`, `firmware`, `modules`, `hardware`, `fleet`, `legacy`, `tools`, `docs`, `.github`, `VERSION`, `README.md`, and `CHANGE_SUMMARY.md`.

Then run:

```powershell
python tools\check_release.py
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 firmware\xiao_esp32s3_sense
```

Commit the clean root with `git add -A` and push `main`. Do not commit the archived backup, `ota_target.h`, Wi-Fi credentials, API keys, `.env` files, or build output.
