# Deskbot v0.0.5.2 — Microphone Loopback and Voice Boundary

This release is based on the verified v0.0.5.1 OTA image and preserves the current XIAO pin map, camera feature, personalities, servo, DRV8833, OLED inversion, and OTA path.

## What changes

- `test mic` and `mic loopback [seconds]` route the onboard Sense PDM microphone to the MAX98357A speaker.
- `audio volume 0..100` sets persistent software volume. Start at 20%.
- The OLED shows a microphone activity bar and a loopback marker.
- The dashboard exposes loopback, mic-meter-only, stop, volume, and wake-name controls.
- `wake name <name>`, `wake on/off`, and `wake simulate` test the engagement boundary.
- Actual spoken-name recognition is intentionally not claimed yet. An amplitude meter cannot identify words; a later local wake-word engine or Xiaozhi/relay adapter must invoke the engagement path after recognition.

## Before committing

From:

```text
C:\Users\msperavali\OneDrive - VE Commercial Vehicles Ltd\Mrunal\Deskbot
```

```powershell
git status --short --branch
git fetch origin --tags
git diff --check
python tools\check_release.py
Get-Content VERSION
Test-Path ".\firmware\xiao_esp32s3_sense\ota_target.h"
```

Expected:

```text
0.0.5.2
False
RELEASE CHECK OK: Deskbot 0.0.5.2; rollout=all; boards=['all']
```

Never commit `ota_target.h`, `secrets.h`, credentials, or build output.

## Commit and push

```powershell
git add .
git diff --cached --check
git commit -m "Deskbot v0.0.5.2: microphone loopback and voice boundary"
git push origin main
```

Do not force-push.

## Wait for GitHub Actions

Open the public repository Actions page and wait for both workflows:

1. **Build all boards** — green.
2. **Publish Deskbot OTA** — green.

Verify the published manifest:

```powershell
Invoke-RestMethod `
  "https://MrunalSwaroop.github.io/deskbot/ota/xiao-esp32s3-manifest.json"
```

It must report:

```text
version : 0.0.5.2
rollout : all
```

The manifest `source` must match the pushed commit.

## Remote update

1. Keep motor power switched off during the firmware update.
2. Open the current dashboard at the exact IP printed by Serial Monitor.
3. Confirm the current board is v0.0.5.1 and `otaConfigured: true`.
4. Press **Restart and check OTA** once.
5. Do not press Reset repeatedly while the board downloads and reboots.
6. Wait for the new Wi-Fi and dashboard messages.
7. Refresh the dashboard and confirm:

```text
firmware: 0.0.5.2
otaState: up_to_date
otaUpdateAvailable: false
```

## Hardware preparation

- Connect the microphone onboard the XIAO Sense; no external mic wiring is required.
- Connect the MAX98357A speaker only between `SPK+` and `SPK-`.
- Do not connect either speaker output to GND.
- Keep the amplifier supply and speaker wiring secure.
- Start with `audio volume 20`.
- If the speaker squeals or clips, immediately run `mic off` or `audio stop`, then reduce volume.
- Use a stable common ground, and keep motor power off during the first audio test.

## Test from Serial Monitor

Set a conservative volume:

```text
audio volume 20
status
```

Run the microphone-to-speaker test:

```text
test mic
```

Speak near the onboard Sense microphone. Expected output includes:

```text
MIC LOOPBACK: active for 60 seconds at volume 20%
MIC level: ... | loopback volume: 20%
```

Stop it:

```text
mic off
```

Change volume:

```text
audio volume 10
audio volume 30
```

Use the meter without speaker output:

```text
mic monitor
mic off
```

Test the engagement path:

```text
wake name rocky
wake on
wake simulate
status
```

Expected behavior is a listening face, a short acknowledgement tone, and a `WAKE: rocky recognized via manual test` message. This is a manual test only; saying “Rocky” aloud is not yet recognized by v0.0.5.2.

## Dashboard test

Open the board dashboard and use:

- **Mic to speaker** or **Loopback 60 s**.
- Set volume percentage.
- **Mic meter only**.
- **Stop loopback**.
- Configure the wake name.
- **Simulate wake**.

The status JSON exposes `micLoopback`, `speakerVolume`, `wakeName`, `wakeEnabled`, and `wakeEngaged`.

## What is intentionally deferred

- Offline spoken wake-word recognition.
- Speech-to-text and text-to-speech.
- Full LLM conversation.
- Direct Xiaozhi account protocol integration.
- Autonomous camera following and motor movement from vision.

Those are separate modules. The roadmap is [`VOICE_LLM_AND_VISION_ROADMAP.md`](VOICE_LLM_AND_VISION_ROADMAP.md). Do not put provider credentials in the XIAO firmware or GitHub repository.

## Recovery

Keep the v0.0.4 tag and the last known-good USB image. If the board stops booting or repeatedly reboots after an OTA update, use the USB recovery procedure rather than repeatedly triggering OTA.
