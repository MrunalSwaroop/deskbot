# Deskbot Modular Architecture

Deskbot has one active controller target today: `boards/xiao-esp32s3-sense/`. The firmware integration lives in `firmware/xiao_esp32s3_sense/`; replaceable behavior belongs under `modules/`.

> A module owns one responsibility, exposes a small command or state interface, and does not directly own unrelated hardware.

| Boundary | Owns | Must not own |
|---|---|---|
| `boards/` | FQBN, board identity, pin map, board-specific limits | Personality behavior or credentials |
| `firmware/` | Hardware integration, boot, dashboard routes, state transitions | Duplicated module implementations |
| `modules/core/` | Shared face states and personality profiles | GPIO initialization |
| `modules/faces/` | OLED rendering, face frames, face-local animation | Motors, Wi-Fi, passwords, OTA |
| `modules/personalities/` | Personality identifiers, descriptions, response direction | Direct pin writes |
| `modules/motion/` | Bounded servo, track, and dance contracts | Hidden power assumptions |
| `modules/audio/` | I2S output contracts and audio limits | Speaker wiring outside hardware docs |
| `modules/voice/` | Push-to-talk, wake-word, STT/TTS backend contracts | A mandatory cloud service |
| `modules/vision/` | Camera capability and presence-detection contracts | Wi-Fi credentials |
| `modules/sensing/` | PDM microphone contracts | Audio amplifier output |
| `modules/network/` | Wi-Fi provisioning and dashboard contracts | Public internet exposure |
| `modules/ota/` | Manifest and rollout contracts | Unverified downloads |

## Add a personality

Create `modules/personalities/<id>_personality.h`. If it needs an original OLED identity, create `modules/faces/<id>_face.h` using the shared `FaceMode` vocabulary. Add the identifier to `modules/core/personality.h`, add a serial/dashboard test path, compile the complete XIAO target, and update `CHANGE_SUMMARY.md`.

Do not copy a real person’s voice or performance. Keep voice playback behind `modules/voice/` and describe it as an original style.

## Add a motor function

Add a bounded command or routine under `modules/motion/`. Every motion function needs a maximum duration, explicit stop path, lifted-robot test, and documented power assumption. The XIAO integration remains responsible for applying the command to the selected board pin map.

## Add audio or voice

Start with `modules/audio/max98357a_output.h` and `tone_test.h`. Validate the MAX98357A and speaker electrically before PCM playback. Add a concrete voice backend under `modules/voice/`; the core firmware must still boot when it is absent.

## Add another board

Create `boards/<profile>/board.json`, `pinmap.h`, and `README.md`, then add a corresponding firmware directory. Do not spread board-specific conditionals through the current XIAO sketch. Add a separate CI matrix entry and distinct device type/artifact.

## Remove a feature

Remove the module, registry entry, test controls, and documentation entry. Run:

```text
python tools/check_release.py
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 firmware/xiao_esp32s3_sense
```

Unrelated hardware modules must remain unchanged.

## Verification contract

Every release must include the firmware, registered modules, board profile, hardware artifacts, workflows, and updated `CHANGE_SUMMARY.md`. The release checker rejects missing required files, invalid board/fleet JSON, local OTA targets, and likely credential patterns.

### References

[1]: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/ "Seeed Studio XIAO ESP32-S3 Getting Started"
[2]: https://docs.github.com/en/actions "GitHub Actions documentation"
[3]: https://docs.github.com/en/pages "GitHub Pages documentation"
