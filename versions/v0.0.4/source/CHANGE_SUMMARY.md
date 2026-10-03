# Rocky-Wall-E POC — Change Summary

This file is included in every ZIP release. It records the requirements addressed, errors corrected, and the files changed in that release.

## Release date

2026-09-27

### OTA 0.0.2 boot-failure correction

**Requirement:** Recover the board after the first OTA image rebooted continuously at `ESP-ROM:esp32s3-20210327`, and prevent the next OTA bootstrap from installing another image built with mismatched XIAO settings.

**Problem corrected:** The GitHub workflow compiled the OTA image with an unqualified FQBN and an unpinned ESP32 core. That did not explicitly reproduce the successful Arduino IDE configuration (`OPI PSRAM`, 8 MB flash, OTA-capable 8 MB partition, Hardware CDC, and QIO settings).

**Changes made:** Pinned GitHub Actions to ESP32 Arduino core `3.3.11`, added the explicit verified XIAO FQBN to both compile workflows, recorded that FQBN in `boards/xiao-esp32s3-sense/board.json`, updated the release checker and local compile instructions, and kept the v0.0.1 base OTA-disabled until a corrected OTA image is published.

**Recovery state:** The board was reflashed successfully and is running `0.0.1-base` with `otaConfigured: false`. Do not create `ota_target.h` until the corrected OTA workflow completes successfully.

### Deskbot v0.0.1 frozen hardware baseline and OTA transition

**Requirement:** Treat the currently working XIAO Sense hardware state as the reproducible Deskbot base, defer expression tuning to a later release, and move to OTA updates without reconnecting USB for every change.

**Changes made:**

- Set the repository semantic version to `0.0.1`.
- Set the normal local firmware identity to `0.0.1-base`.
- Added `docs/BASELINE_V0.0.1.md` documenting the verified hardware scope, known expression limitations, acceptance checks, and the remote-update transition.
- Configured `ota_target.example.h` for the public `MrunalSwaroop/deskbot` GitHub Pages manifest and `0.0.1-ota-bootstrap` one-time bootstrap image.
- Updated the root README, firmware guide, and fleet OTA guide so the user performs one final USB OTA-capable upload; future releases are GitHub-only while USB remains recovery-only.
- Added the remote `restart` command and dashboard **Restart and check OTA** button so an OTA-enabled board can check the newly published manifest without a USB cable.

**Important limitation:** A firmware build with OTA disabled cannot enable OTA without a physical upload. The one-time OTA bootstrap is therefore required before cable-free updates can begin.

**Known baseline issue:** Expression artwork and mappings are not frozen as final behavior in v0.0.1. They remain modular under `modules/faces/` and will be corrected in the next higher release.

### Local dashboard JavaScript and control-path correction

**Requirement:** The browser dashboard must show live status and operate the same face, personality, servo, motor, camera, microphone, audio, and Wi-Fi functions that already work from Serial Monitor.

**Problem corrected:** The generated dashboard script placed C++ newline escapes directly into JavaScript string literals. The browser rejected the whole script, leaving status at `Loading...` and making every button appear non-functional even though the `/cmd` serial-equivalent handler was present.

**Changes made:** Escaped the newline characters for the browser correctly, added HTTP/request error handling, added a cache-busting status request, and preserved the single shared command path between Serial and dashboard controls.

**Verification:** The generated script now remains valid JavaScript, the clean-root XIAO sketch compiles with the dashboard fix, and `/health`, `/api/status`, and `/cmd?c=...` remain registered routes.

### Rocky body-language, expressive Isabella, and first-boot connectivity correction

**Requirement:** Before committing the Deskbot repository, verify that the active clean-root firmware really uses the supplied Rocky body-language files, that Isabella speaking is animated rather than static, that `sad`, `curious`, and `thinking` are distinct, and that the XIAO can be brought online with a visible local dashboard for camera, mic, servo, motor, OLED, and audio testing.

**Problem corrected:** The active sketch still selected the older procedural Rocky renderer, Isabella’s automatic mapper called its state setter on every OLED refresh, and first-boot Wi-Fi instructions did not make the setup-AP/dashboard path explicit enough.

**Changes made:**

- Added `modules/faces/rocky_body_language.h`, adapted from the supplied Rocky behavior source and isolated behind the shared face module boundary.
- Connected `personality rocky` and explicit `rocky idle|curious|greeting|listening|forward|back|pace|come|go|sleep|wake|dance` commands to the supplied body poses.
- Added stable Rocky animation ticking without taking ownership of the main Wi-Fi, motor, camera, microphone, or audio layers.
- Added Isabella derived states and animated overlays for curious, thinking, sad, laughing, surprised, and speaking; repeated refreshes no longer reset the active state.
- Added `secrets.h.example` for an optional direct first-boot 2.4 GHz connection while keeping the real `secrets.h` ignored.
- Made setup AP addressing deterministic at `192.168.4.1` and print `DASHBOARD: http://...` for both AP and normal Wi-Fi modes.
- Added `/health` (`deskbot-ok`) for a fast local connectivity check before opening the full dashboard.
- Added microphone noise-floor removal and smoothing so `mic monitor` reports activity instead of raw jitter.
- Expanded the firmware guide with the exact serial/dashboard verification order and troubleshooting notes.

**Verification:** The clean-root XIAO sketch compiled successfully with Arduino CLI, ESP32 core 3.3.11, camera, PDM microphone, MAX98357A audio, servo, DRV8833, Wi-Fi, dashboard, Rocky, Isabella, and Spartan modules present. Hardware must still be checked on the user’s board before commit: open the printed dashboard URL, run `test camera`, `mic monitor`, `test servo`, isolated motor tests, and the audio tone test with motor power disconnected where appropriate.

### Clean Deskbot-root modular platform

**Requirement:** Use `C:\Users\msperavali\OneDrive - VE Commercial Vehicles Ltd\Mrunal\Deskbot` itself as the repository root. Keep the XIAO firmware scalable so personalities, faces, motor routines, voice, audio, sensing, networking, board profiles, and OTA rollout can be added or removed independently. Flash the board initially over USB, then update selected boards remotely through GitHub.

**Changes made:**

- Removed the active dependency on the nested `xiao_bringup/` directory. The active target is now `firmware/xiao_esp32s3_sense/xiao_esp32s3_sense.ino`.
- Created the clean `boards/`, `firmware/`, `modules/`, `hardware/`, `fleet/`, `tools/`, and `legacy/` boundaries.
- Moved the XIAO pin map to `boards/xiao-esp32s3-sense/pinmap.h` and added a board profile with Arduino IDE settings.
- Added shared core contracts, replaceable face and personality modules, motion/audio/voice/vision/sensing/network/OTA module contracts, and module README boundaries.
- Moved wiring diagrams and pin tables into `hardware/wiring/`.
- Kept earlier UNO and gateway experiments under `legacy/` so they remain available without being active firmware targets.
- Renamed the GitHub workflows to `build-all-boards.yml`, `publish-ota.yml`, and `release-check.yml`, all using the clean paths.
- Updated the release checker, GitHub Pages artifact name, semantic version source, fleet policy, and OTA documentation.
- Added `docs/MIGRATE_EXISTING_DESKBOT.md` with a non-destructive one-time command to archive stale nested folders before the clean first commit.

**Correction:** Arduino CLI requires the primary `.ino` filename to match its sketch directory. The active sketch is therefore `xiao_esp32s3_sense.ino`, not a differently named `.ino` inside that directory.

**Verification:** The release checker passed. The clean XIAO firmware compiled successfully with Arduino CLI and ESP32 core 3.3.11. No local `ota_target.h`, secrets, or build output was retained.

### Modular Deskbot base and fleet OTA release contract

**Requirement:** Use `C:\Users\msperavali\OneDrive - VE Commercial Vehicles Ltd\Mrunal\Deskbot` as the single working repository. Keep personalities, faces, motor functions, audio, voice, networking, and hardware configuration replaceable. Allow each verified GitHub version to update all selected XIAO boards without repeating manual redesign work.

**Changes made:**

- Added `VERSION` as the single semantic release source.
- Added `boards/xiao-esp32s3-sense/board.json` as the active board profile.
- Added `fleet/selected_boards.json` for `all` or `selected` OTA rollout.
- Moved Isabella, Spartan, and Rocky display assets into `modules/faces/`.
- Added `docs/MODULAR_ARCHITECTURE.md`, `modules/README.md`, and `docs/RELEASE_AND_FLEET_OTA.md`.
- Added unique XIAO board IDs to serial status and `/api/status`.
- Added manifest targeting so an unselected board refuses the update while selected boards accept the same newer release.
- Updated both GitHub Actions workflows to track modular files, board profiles, fleet policy, and version changes.
- Published versioned OTA release copies and SHA-256 checksums in the Pages artifact.

**Verification:** The local USB build compiled successfully. A temporary OTA-enabled build compiled successfully. JSON configuration files parsed successfully. The fleet manifest logic, board ID reporting, workflow paths, ignored OTA target, and secret exclusion checks passed.

### Permanent OTA build separation and ESP32 3.3.7 compatibility

**Requirement:** Stop the XIAO from repeatedly overwriting a USB recovery upload and make the local Arduino IDE build compatible with the installed ESP32 core 3.3.7.

**Changes made:** Added `ROCKY_OTA_ENABLED`, disabled OTA by default in local USB builds, enabled it explicitly only in the GitHub-generated release target, and corrected the ESP32 3.3.7 I2S constructor and buffer write. This separates local hardware verification from GitHub OTA releases.

**Verification:** Local no-target build compiled successfully; temporary OTA-enabled build compiled successfully; no local `ota_target.h` was retained in the package.

### Desktop-pet OLED reference integration

**Requirement:** Use the referenced `desktop pet oled` task so character identity, expression state, animation, and hardware actions remain modular instead of making the XIAO sketch a single entangled renderer.

**Changes made:** Imported the reusable Isabella module and its seven 128x64 state bitmaps, imported the independent procedural Spartan commander renderer, connected both to the existing `personality` and `face` state model, and added direct `isabella <state>` and `spartan <state>` commands. The Rocky bump frame table is included as the next dance-animation source but does not yet replace the currently verified dance behavior.

**Compatibility:** The imported modules are display-only. Camera, PDM microphone, MAX98357A audio, servo, DRV8833 motors, Wi-Fi dashboard, and OTA code remain separate. The existing Calm, Rocky, and Engineer procedural faces remain unchanged.

**Files added:** `xiao_bringup/isabella_character.h`, `xiao_bringup/isabella_state_bitmaps.h`, `xiao_bringup/spartan_character.h`, and `xiao_bringup/rocky_bump_frames.h`.

**Verification:** Compile the XIAO sketch and test `personality isabella`, `isabella happy`, `isabella speaking`, `personality spartan`, `spartan salute`, `spartan thinking`, and `spartan victory` before reconnecting motor or external actuator power.

### GitHub repository name correction

**Requirement:** Use `deskbot` as the GitHub repository name instead of `rocky-walle-poc`.

**Changes made:** Updated the OTA target example, OTA guide, and root project instructions to use the GitHub Pages path `/deskbot/`. The local historical folder and ZIP filename remain unchanged so existing local references continue to work.

### GitHub OTA bootstrap handoff correction

**Requirement:** Upload the firmware once over USB, verify the hardware, then move to GitHub-controlled remote firmware updates.

**Correction:** The firmware bootstrap is `0.3.0`, but the prior Pages workflow generated `0.1.<run-number>` versions. Those versions would be treated as older and would not install. The workflow now generates `0.3.<run-number>` versions, and `ota_target.example.h` plus the OTA guide now match that sequence.

**Verification:** Before OTA, test OLED, servo, individual motors with the robot lifted, microphone, camera, MAX98357A tone, dashboard, and Wi-Fi. OTA must be tested only after the manifest-enabled USB image is running and the device has a stable 2.4 GHz connection.

**Files changed:** `.github/workflows/publish-ota-pages.yml`, `xiao_bringup/ota_target.example.h`, `docs/xiao_ota_and_reference_integration.md`, and `CHANGE_SUMMARY.md`.

### Complete functional XIAO firmware with MAX98357A tone test

**Requirement:** After configuring the adjacent pins, provide the complete uploadable XIAO ESP32-S3 Sense firmware rather than only wiring tables. Preserve modular single-function tests and make the new MAX98357A branch testable.

**Changes made:**

- Restored the repository-neutral `device_config.h`, `ota_service.h`, and `ota_pages_root_ca.h` support files required by the existing firmware and OTA workflow.
- Added a second I2S output instance on the MAX98357A pins `DIN=GPIO2/D1`, `BCLK=GPIO3/D2`, and `LRC/LRCK=GPIO4/D3`, while leaving the Sense PDM microphone on GPIO42/GPIO41.
- Added `test audio`, `audio tone [Hz] [ms]`, and `audio stop` commands.
- Added non-blocking 16 kHz stereo square-wave tone generation at conservative amplitude for the first speaker test.
- Added audio readiness and tone state to Serial status, `/api/status`, and dashboard controls.
- Kept OLED personalities, servo head movement, DRV8833/N90 motor controls, Wi-Fi provisioning, local dashboard, camera snapshot/live preview, PDM microphone monitoring, head-state behavior, dance, and optional OTA in the same modular sketch.
- Documented the complete uploadable file set and audio test procedure.

**Errors or risks addressed:** Restored missing headers that prevented the complete sketch from compiling; kept microphone RX and amplifier TX on separate I2S controller instances; avoided adding voice/LLM/TTS behavior before the electrical speaker test is stable; and retained the bridge-tied speaker warning.

**Verification:** Arduino CLI 1.5.1 with ESP32 core 3.3.11, Adafruit GFX 1.12.6, Adafruit SSD1306 2.5.17, and ESP32Servo 3.2.1 compiled the complete `XIAO_ESP32S3` sketch successfully. The build used 32% program storage and 18% dynamic memory.

**Files changed:**

- `xiao_bringup/xiao_bringup.ino`
- `xiao_bringup/device_config.h`
- `xiao_bringup/ota_service.h`
- `xiao_bringup/ota_pages_root_ca.h`
- `xiao_bringup/README.md`
- `README.md`
- `CHANGE_SUMMARY.md`

### Adjacent XIAO header map and compact part tables

**Requirement:** Provide table-format connections for every part and simplify assembly by using neighboring XIAO pins wherever possible.

**Changes made:**

- Remapped the external signal groups into one contiguous header block: D0 servo, D1-D3 MAX98357A, D4-D5 OLED, and D6-D9 DRV8833.
- Kept the Sense PDM microphone on its internal GPIO42/GPIO41 connection and the Sense camera on its internal camera connector pins.
- Added `COMPACT_PIN_TABLES.md` with one table each for the XIAO, OLED, servo, DRV8833, both motors, MAX98357A, microphone, camera, and power rails.
- Updated the firmware configuration, complete wiring diagram, simplified physical layout, assembly harness, pin diagram, and hardware guide to match the new map.

**Errors or risks addressed:** Removed unnecessary long cross-over signal wires; avoided camera and microphone pin conflicts; kept the SD bus deliberately unused because the current motor map occupies GPIO7/GPIO8; and preserved separate motor power, common ground, and bridge-tied speaker safety rules.

**Adjacent external signal map:** `D0` servo, `D1` amplifier DIN, `D2` amplifier BCLK, `D3` amplifier LRC/LRCK, `D4/D5` OLED SDA/SCL, and `D6/D7/D8/D9` DRV8833 AIN1/AIN2/BIN1/BIN2.

**Verification:** The adjacent pin definitions, compact tables, diagrams, and stale-map checks passed. Arduino CLI was installed and the full sketch compile was attempted; it stopped before sketch compilation because the current working tree is missing inherited OTA helper headers `device_config.h`, `ota_service.h`, and `ota_pages_root_ca.h`. This is independent of the pin remap and is recorded rather than hiding the limitation.

**Files changed:**

- `xiao_bringup/xiao_config.h`
- `xiao_bringup/COMPACT_PIN_TABLES.md`
- `xiao_bringup/complete_wiring.mmd`
- `xiao_bringup/complete_wiring.png`
- `xiao_bringup/wiring.mmd`
- `xiao_bringup/wiring.png`
- `xiao_bringup/SIMPLE_ASSEMBLY_LAYOUT.mmd`
- `xiao_bringup/SIMPLE_ASSEMBLY_LAYOUT.png`
- `xiao_bringup/ASSEMBLY_LAYOUT.md`
- `xiao_bringup/PIN_DIAGRAM.md`
- `xiao_bringup/README.md`
- `README.md`
- `CHANGE_SUMMARY.md`

### Simplified physical assembly layout

**Requirement:** The complete schematic was still difficult to assemble physically. Provide a layout showing where each module should sit and reduce unnecessary wiring and additional circuitry wherever possible.

**Changes made:**

- Added a physical placement layout with the OLED at the front, XIAO directly behind it, the pan servo below the head, the DRV8833 at the rear between the motors, and the MAX98357A beside the speaker.
- Reduced the recommended external power arrangement to one regulated 5 V rail for the servo and MAX98357A, one separate motor VM rail, and one shared ground bus.
- Added exact point-to-point harness tables for the OLED, servo, DRV8833, motors, MAX98357A, speaker, and power rails.
- Explicitly documented that the Sense camera and PDM microphone require no loose wires.
- Added a staged power-up order so each subsystem can be tested independently.

**Errors or risks addressed:** Removed the need for a level shifter, extra controller, resistor network, or separate enable board in this bring-up; separated high-current motor wires from logic and audio signal wires; and retained the speaker bridge-output and common-ground safety rules.

**Files changed:**

- `xiao_bringup/ASSEMBLY_LAYOUT.md`
- `xiao_bringup/SIMPLE_ASSEMBLY_LAYOUT.mmd`
- `xiao_bringup/SIMPLE_ASSEMBLY_LAYOUT.png`
- `xiao_bringup/README.md`
- `README.md`
- `CHANGE_SUMMARY.md`

### Complete XIAO pin diagram and confirmed MAX98357A wiring

**Requirement:** Produce a complete user-facing circuit and pin diagram for the active XIAO ESP32-S3 Sense robot, explicitly including the SSD1306 OLED and the two-channel DRV8833 with both N90 motors. Add the newly acquired I2S amplifier wiring without creating conflicts with the camera, PDM microphone, servo, or motor pins.

**Clarification:** The user first reported the amplifier marking as `MAX98756A`, then confirmed that it is `MAX98357A`. The audio branch is now documented for a MAX98357A I2S mono amplifier.

**Changes made:**

- Added the complete editable Mermaid wiring diagram with XIAO GPIOs, OLED I2C, pan servo, DRV8833 A/B inputs and outputs, nSLEEP/SLP, VM, both N90 motors, separate power domains, common ground, Sense camera, onboard PDM microphone, and the MAX98357A audio branch.
- Added the complete rendered PNG diagram for direct visual reference.
- Added a complete pin table with connection checklists, speaker bridge-output warnings, power rules, reserved GPIOs, test order, and external references.
- Kept the existing `wiring.mmd` and `wiring.png` filenames synchronized as compatibility copies.
- Updated the XIAO guide and root project index to link the new artifacts.

**Errors or risks addressed:** Resolved the amplifier-name ambiguity before finalizing the audio wiring; prevented speaker outputs from being tied to ground; preserved common-ground and separate-power requirements; and documented that GPIO7/8/9 cannot simultaneously be used for the current audio/motor map and Sense SD interface.

**Files changed:**

- `xiao_bringup/complete_wiring.mmd`
- `xiao_bringup/complete_wiring.png`
- `xiao_bringup/PIN_DIAGRAM.md`
- `xiao_bringup/wiring.mmd`
- `xiao_bringup/wiring.png`
- `xiao_bringup/README.md`
- `README.md`
- `CHANGE_SUMMARY.md`

## Latest requirements and fixes

### XIAO OTA and referenced-task integration

**Requirement:** Use the referenced ESP32 CI/CD/OTA task and the desktop-pet OLED task as the next knowledge base while keeping the current XIAO hardware firmware, dashboard, camera, microphone, servo, and DRV8833 behavior intact.

**Changes made:**

- Adapted the verified `DB_PetBot` XIAO HTTPS manifest/update service into `xiao_bringup/`.
- Reused the existing `rocky-xiao` Wi-Fi preferences instead of creating a second credential store.
- Added firmware version and OTA-manifest status to `/api/status` and the dashboard.
- Added a XIAO-only GitHub Actions compile workflow and a GitHub Pages publishing workflow for the raw ESP32 application binary and manifest.
- Added the local `ota_target.example.h`, GitHub Pages root certificate, OTA bootstrap notes, device matrix, and release references.
- Preserved the modular character/state boundary from the desktop-pet OLED work so richer Isabella, Rocky, and Spartan frame tables can be added without coupling them to OTA.

**Errors or risks addressed:** Avoided using the UNO OTA container for an ESP32-S3 image; prevented OTA from creating a second Wi-Fi provisioning path; ensured equal or older versions are not reinstalled; and kept USB upload as the recovery path. The local dashboard remains private-network-only.

**Files changed:**

- `xiao_bringup/xiao_bringup.ino`
- `xiao_bringup/ota_service.cpp`
- `xiao_bringup/ota_service.h`
- `xiao_bringup/device_config.h`
- `xiao_bringup/ota_pages_root_ca.h`
- `xiao_bringup/ota_target.example.h`
- `.github/workflows/build-firmware.yml`
- `.github/workflows/publish-ota-pages.yml`
- `docs/xiao_ota_and_reference_integration.md`
- `xiao_bringup/README.md`
- `README.md`
- `.gitignore`
- `docs/referenced_tasks/KNOWLEDGE_BASE.md`
- `docs/referenced_tasks/DEVICE_MATRIX.md`
- `docs/referenced_tasks/OTA_BOOTSTRAP.md`
- `docs/referenced_tasks/RELEASES.md`
- `scripts/bin2ota.py`

## Previous requirements retained

### XIAO Stage 4: distinct faces, motor calibration, live camera, microphone, head states, and dance

**Requirement:** Calm, Isabella, and Rocky looked too similar; forward and right motion still needed calibration; Isabella’s dashboard button needed to remain blue; the camera needed a live-view mode; and the next bring-up step needed microphone, head-state integration, and dance controls.

**Changes made:**

- Added stronger Calm, Rocky, and Isabella OLED differences through eye geometry, pupil size, eyebrow motifs, accents, and resting-mouth shapes.
- Changed Isabella’s dashboard button styling to blue.
- Added persisted runtime motor inversion commands for each track, allowing forward/right calibration without recompiling:
  `motor invert left on/off` and `motor invert right on/off`.
- Added low-rate browser live camera mode at `/camera/live`, implemented as repeated JPEG snapshots so the XIAO web server remains responsive.
- Added PDM microphone initialization using the XIAO Sense GPIO42 clock and GPIO41 data pins.
- Added `test mic`, microphone monitoring, level output, and dashboard microphone status.
- Added state-driven servo head poses for listening, thinking, working, curious, sad, and error states.
- Added a non-blocking dance routine combining pan, expressions, and short DRV8833 movements.

**Errors or risks addressed:** Removed the need to guess compile-time motor inversion; avoided a blocking MJPEG stream that could starve robot controls; and made microphone, head, and dance behavior independently testable before adding speech recognition or LLM audio.

**Files changed:**

- `xiao_bringup/xiao_bringup.ino`
- `xiao_bringup/README.md`

### Windows ESP32 camera-handler compatibility fix

**Error observed:** Windows Arduino compilation failed at `NetworkClient &client = server.client();` because the selected ESP32 WebServer variant returned a temporary `WiFiClient` object.

**Change made:** Changed the camera handler to store the result as `WiFiClient client = server.client();`, which compiles with both the current ESP32 WebServer API and the Windows ESP32 package variant reported by the user. Added documentation explaining the duplicate user-installed `WebServer` and `WiFi` libraries shown in the build output.

**Verification:** Recompiled successfully with Arduino CLI 1.5.1, ESP32 core 3.3.11, and the `XIAO_ESP32S3` target. The only remaining output is the existing ESP32Servo legacy-MCPWM warning.

**Files changed:**

- `xiao_bringup/xiao_bringup.ino`
- `xiao_bringup/README.md`

### XIAO Stage 3: motor-direction diagnosis, Rocky/Isabella identities, and camera preview

**Requirement:** The combined DRV8833 forward command did not produce useful forward motion; rename the old Musical personality to Rocky; rename the Jolly personality to Isabella with a softer animated AI-girl style; and begin testing the XIAO Sense camera.

**Changes made:**

- Set the default right-track inversion for a conventional mirrored tracked chassis while keeping both motor inversion flags editable in `xiao_config.h`.
- Added `test left` and `test right` commands so each DRV8833 channel can be tested independently before combined forward/back/turn commands.
- Renamed the visible personalities to `rocky` and `isabella`; retained `musical`, `jolly`, and `anime` as legacy aliases.
- Added Isabella’s softer rounded eyes, bow/sparkle accents, gentle mouth, quicker blink rhythm, and subtle randomized mood changes.
- Added XIAO Sense camera initialization using the official Sense DVP pin map and PSRAM-backed JPEG capture.
- Added `/camera.jpg`, a dashboard camera preview, `test camera`, camera status reporting, and camera-specific troubleshooting.

**Errors or risks addressed:** Separated motor polarity/wiring faults from combined-drive logic; prevented the old personality labels from persisting in the dashboard; and reserved the camera’s occupied GPIOs so future microphone/audio wiring does not reuse them.

**Files changed:**

- `xiao_bringup/xiao_bringup.ino`
- `xiao_bringup/xiao_config.h`
- `xiao_bringup/README.md`
- `README.md`

### XIAO Stage 2: DRV8833, distinct task states, and Jolly personality

**Requirement:** Continue on the XIAO after the OLED, Wi-Fi, and servo worked; replace the earlier L293D motor assumptions with the user’s DRV8833 two-channel driver; make thinking different from working; and add a high-energy, happy, mood-changing Jolly anime-inspired personality.

**Changes made:**

- Replaced the L293D enable/direction model with direct DRV8833 AIN1/AIN2 and BIN1/BIN2 PWM control.
- Added `xiao_config.h` as the single editable hardware configuration file, including pin reservations, servo limits, motor inversion flags, safety timeout, and compile-time conflict checks.
- Added separate `thinking...` and `executing...` OLED labels and animations.
- Added `personality jolly`, with `anime` and `isabella` aliases, rounded eyes, sparkles, cheerful mouth animation, and randomized happy/curious/laughing/surprised idle moods.
- Added single-function commands and dashboard buttons for OLED, servo, motor, and Wi-Fi tests.
- Added a DRV8833 wiring table, power guidance, circuit diagram source, and modular next-stage plan.

**Errors or risks addressed:** Corrected the driver pin model so the firmware no longer treats DRV8833 as an L293D with separate enable pins; reduced future pin-conflict risk by reserving microphone, audio, camera, OLED, servo, and SD pins; and removed the misleading shared `working...` label from thinking.

**Files changed:**

- `xiao_bringup/xiao_bringup.ino`
- `xiao_bringup/xiao_config.h`
- `xiao_bringup/README.md`
- `xiao_bringup/wiring.mmd`
- `README.md`

### 0. XIAO ESP32-S3 Sense bring-up

**Requirement:** Move the hardware bring-up from the UNO R4 to the XIAO board itself and test the OLED, pan servo, L293D/N90 motors, Wi-Fi, and browser dashboard before adding voice features.

**Change made:** Added `xiao_bringup/xiao_bringup.ino` with XIAO pin definitions, safe servo and motor controls, face states, Wi-Fi station/AP provisioning, NTP time, and a local dashboard. Added `xiao_bringup/README.md` with wiring, power safety, upload, Wi-Fi, test commands, reserved Sense pins, and the future standalone voice roadmap.

**Error or risk addressed:** Prevented accidental reuse of Sense microphone, amplifier, camera, and OLED pins; documented the external-power/common-ground requirement; and added a setup AP so Wi-Fi credentials do not need to be compiled into firmware.

**Files changed:**

- `xiao_bringup/xiao_bringup.ino`
- `xiao_bringup/README.md`
- `README.md`

### 0.1 XIAO runtime assertion after reboot

**Error observed:** The XIAO rebooted with `assert failed: xQueueSemaphoreTake queue.c:1709 (( pxQueue ))`. Backtrace decoding against the compiled ELF pointed into the ESP32 networking/WebServer path while the dashboard server was being finalized.

**Change made:** Kept route registration in `setupRoutes()` but moved `server.begin()` until after `connectStoredWiFi()` has initialized either station mode or the setup access point. This prevents the HTTP server from starting before the network interface exists.

**File changed:** `xiao_bringup/xiao_bringup.ino`

### 1. Recognizable Engineer personality identity

**Requirement:** The Engineer personality needed to look like Einstein rather than only changing the eyes and mouth.

**Change made:** Added a full-screen monochrome Engineer identity card with Einstein-inspired wild hair, round glasses, moustache, nose, bow tie, and `EINSTEIN` / `CURIOUS MIND` labels. The design is stylized OLED pixel art rather than a photographic image.

**File changed:** `uno_r4_current_desk_buddy/uno_r4_current_desk_buddy.ino`

### 2. Recognizable Spartan personality identity

**Requirement:** The Spartan personality needed to look like a 300-inspired ancient Spartan soldier rather than only changing the eyes and mouth.

**Change made:** Added a full-screen monochrome Spartan identity card with plume, helmet, face guard, eye slit, cheek guards, shield-like lower silhouette, and `SPARTAN` / `DISCIPLINE` labels.

**File changed:** `uno_r4_current_desk_buddy/uno_r4_current_desk_buddy.ino`

### 3. Personality switch reveal

**Requirement:** Switching personality should visibly announce the new personality.

**Change made:** Personality changes now show the identity card for approximately 2.4 seconds, then return automatically to the normal expressive face. The implementation is non-blocking so Wi-Fi, dashboard, servo, and other robot functions continue to run.

**File changed:** `uno_r4_current_desk_buddy/uno_r4_current_desk_buddy.ino`

### 4. Personality-matching RGB LED

**Requirement:** The external RGB LED should emit the color associated with the selected personality.

**Change made:** Added immediate personality colors: blue for Calm, violet for Musical, cyan for Engineer, white for Quiet, and red-orange for Spartan. Active states such as thinking, listening, speaking, notification, error, and dance temporarily take priority. The personality color returns when the robot becomes idle.

**File changed:** `uno_r4_current_desk_buddy/uno_r4_current_desk_buddy.ino`

### 5. Direct LLM authentication error

**Error sorted:** The UNO received `HTTP/1.1 401 Unauthorized` because a Manus API key was being sent to `api.openai.com` through the direct OpenAI-compatible path.

**Change made:** Added Serial Monitor authentication hints explaining that the direct UNO path requires an OpenAI-compatible key, while Manus requires the relay path with the `x-manus-api-key` header. The Manus key must not be placed in `arduino_secrets.h`.

**Files changed:**

- `uno_r4_current_desk_buddy/uno_r4_current_desk_buddy.ino`
- `uno_r4_current_desk_buddy/arduino_secrets.h.example`
- `integration_relay/relay_server.py`
- `integration_relay/README.md`
- `docs/non_destructive_desk_buddy.md`

## Earlier major fixes retained in this release

- Fixed the dashboard status API response and added visible browser error diagnostics.
- Added HTTP response content lengths and client timeouts.
- Added Wi-Fi DHCP/IP diagnostics and port 8080 dashboard fallback.
- Added 0–180 degree pan control with 90 degrees as center.
- Added L293D/N90 motor controls with default-off and timed safety behavior.
- Added face states for idle, listening, thinking, speaking, happy, dance, notification, error, sleep, laugh, surprise, sad, curious, and worried.
- Added personality-specific movement and speaking animation profiles.
- Added time, weather, notifications, Pomodoro, water reminders, daily activity tracking, and optional head-following.
- Added the embedded browser dashboard with buttons instead of requiring Serial Monitor commands.
- Preserved the existing UNO R4 WiFi architecture without reflashing or replacing the onboard bridge firmware.

## Verification performed

- Arduino sketch structural checks passed.
- XIAO bring-up sketch brace-balance and feature checks passed.
- XIAO bring-up guide was cleaned so it contains one practical reference section rather than repeated blocks.
- Arduino CLI 1.5.1, ESP32 core 3.3.11, and the `XIAO_ESP32S3` target compile the corrected sketch successfully.
- The modular Stage 2 XIAO sketch compiles with Arduino CLI 1.5.1, ESP32 core 3.3.11, and the `XIAO_ESP32S3` target.
- The Stage 3 XIAO camera/personality/motor-diagnostic sketch compiles with Arduino CLI 1.5.1, ESP32 core 3.3.11, and the `XIAO_ESP32S3` target.
- Camera DVP mapping matches Seeed’s XIAO ESP32-S3 Sense documentation and the installed ESP32 CameraWebServer example.
- Feature checks confirm Rocky/Isabella identifiers, camera route, isolated motor tests, right-track inversion, and no stale enum/config identifiers.
- Configuration-header compile-time checks pass for unique pins and reserved future peripherals.
- ESP32Servo emits legacy MCPWM deprecation warnings only; no compilation errors remain.
- C++ brace-balance check passed.
- Python relay syntax check passed.
- ZIP was rebuilt without private `arduino_secrets.h`, `.env`, or Python cache files.
- OTA-disabled XIAO compile passed with Arduino CLI 1.5.1, ESP32 core 3.3.11, and `XIAO_ESP32S3`.
- OTA-enabled XIAO compile passed with temporary `ota_target.h`; the temporary target file was removed afterward.
- The local workflows were checked for XIAO sketch paths, GitHub Pages publishing, manifest generation, and artifact paths.
- Complete Mermaid diagram rendered successfully to a 3120 × 2452 PNG and was visually checked for OLED, DRV8833, motor, servo, audio, microphone, camera, power, common-ground, and do-not-connect blocks.
- Confirmed final diagram and pin table contain `MAX98357A`, `GPIO7` BCLK, `GPIO4` LRC/LRCK, `GPIO2` DIN, `SPK+`, and `SPK−` without the provisional `MAX98756A` label.

## 2026-09-15 packaging requirement

**Requirement:** Every future ZIP must contain a file explaining what changed, which requirement was addressed, and which error was corrected.

**Change made:** Added this `CHANGE_SUMMARY.md` file to the project root and documented the rule in `README.md`. Future releases must update this file before packaging.

**Files changed:**

- `CHANGE_SUMMARY.md`
- `README.md`

## Future release rule

Whenever this project is packaged as a ZIP, update this file first with:

1. The date.
2. The user's requirement or the error observed.
3. The change made to address it.
4. The files changed.
5. The verification performed.

Never place private API keys, Wi-Fi passwords, or local `.env` files in the ZIP.
