# XIAO Rocky Desk Buddy — modular Stage 2 bring-up

This stage keeps everything on the **Seeed XIAO ESP32-S3 Sense** and tests the hardware one function at a time:

1. OLED face.
2. Pan servo.
3. DRV8833 plus two N90 motors.
4. Wi-Fi provisioning and local dashboard.
5. Thinking/working state distinction.
6. Calm, Rocky, Engineer, Spartan, and soft animated Isabella personalities.
7. XIAO Sense camera initialization, snapshot, and low-rate live preview.
8. XIAO Sense PDM microphone level monitoring.
9. State-driven head movement and a safe desk-robot dance routine.

The voice and LLM stages come only after this base is stable. OTA is now included as an optional, USB-bootstrap-first update layer.

## Files

| File | Purpose |
|---|---|
| `xiao_bringup.ino` | Main modular bring-up firmware |
| `xiao_config.h` | Pin map, reserved pins, safety limits, and motor inversion settings |
| `PIN_DIAGRAM.md` | Complete connection table and bring-up checklist |
| `COMPACT_PIN_TABLES.md` | One compact table per part using adjacent XIAO headers |
| `complete_wiring.mmd` | Complete circuit/wiring diagram source |
| `complete_wiring.png` | Rendered complete circuit/wiring diagram |
| `ASSEMBLY_LAYOUT.md` | Simplified physical placement and point-to-point harness |
| `SIMPLE_ASSEMBLY_LAYOUT.mmd` | Simplified physical layout source |
| `SIMPLE_ASSEMBLY_LAYOUT.png` | Simplified physical layout image |
| `wiring.mmd` | Legacy diagram filename, kept in sync with the complete diagram |

The complete uploadable source is `xiao_bringup.ino`. Keep `xiao_config.h`, `device_config.h`, `ota_service.h`, `ota_service.cpp`, `ota_pages_root_ca.h`, and `xiao_bringup.ino` together in the same Arduino sketch folder.

Change hardware pin choices only in `xiao_config.h`. The firmware intentionally contains no Wi-Fi password or API key.

## Safety and power architecture

Use separate power domains:

- **XIAO:** USB-C or a clean regulated 5 V input as specified by Seeed.
- **OLED:** XIAO 3V3 and GND.
- **Servo:** external regulated 5 V supply; never XIAO 3V3.
- **DRV8833 motor VM:** separate motor battery or regulated motor supply suitable for the N90 motors.
- **MAX98357A-style amplifier:** separate suitable audio supply; 5 V is recommended for the first confirmed-board test.
- **Ground:** XIAO GND, servo-supply GND, and motor-driver GND must be connected together.

Never feed 5 V into a XIAO GPIO. Keep the robot lifted for the first motor tests. Add a physical motor power switch. If the XIAO resets when the servo or motors start, stop testing and fix the power system rather than adding software delays.

The DRV8833 is a dual H-bridge. It can control two brushed DC motors, but motor current, supply voltage, heat, and stall current must stay within the specific carrier-board and motor limits. A bulk capacitor close to the driver motor supply is recommended; follow the carrier-board documentation for its value and placement.

## Wiring

### OLED

| OLED | XIAO |
|---|---|
| GND | GND |
| VCC | 3V3 |
| SDA | GPIO5 / D4 |
| SCL | GPIO6 / D5 |

The firmware uses SSD1306 address `0x3C`. If the display is blank, scan I2C and try `0x3D` in `xiao_config.h`.

### Pan servo

| Servo wire | Connection |
|---|---|
| Signal | GPIO1 / D0 |
| Red | External regulated 5 V |
| Brown/black | External supply GND |
| Supply GND | Connected to XIAO GND |

Begin at 90°, then test 60° and 120°. Although the firmware accepts 0–180°, the physical mechanism may not. Reduce `ROCKY_SERVO_MIN_ANGLE` and `ROCKY_SERVO_MAX_ANGLE` in `xiao_config.h` if the linkage approaches a hard stop.

### DRV8833

The DRV8833 does **not** use separate L293D-style enable pins. This firmware sends PWM directly to the four H-bridge input pins:

| DRV8833 | XIAO | Purpose |
|---|---:|---|
| AIN1 | GPIO43 / D6/TX | Left motor direction/PWM input 1 |
| AIN2 | GPIO44 / D7/RX | Left motor direction/PWM input 2 |
| BIN1 | GPIO7 / D8 | Right motor direction/PWM input 1 |
| BIN2 | GPIO8 / D9 | Right motor direction/PWM input 2 |
| nSLEEP / SLP | 3V3 | Must be HIGH for this firmware |
| GND | GND | Common signal ground |
| VM | External motor supply | Motor power input |
| AOUT1/AOUT2 | Left N90 | Motor A |
| BOUT1/BOUT2 | Right N90 | Motor B |
| nFAULT | Not connected | Optional future diagnostic input |

For a typical DRV8833 carrier, connect the carrier’s logic supply as its documentation specifies. Some breakout boards expose `VCC`, while others derive logic internally; do not assume the labels are identical between boards. Confirm the exact board marking before applying power.

Do not connect the motor supply to XIAO 3V3. Do not connect a motor directly to the XIAO. If a motor turns in the wrong direction, either swap its two motor wires or change one of these configuration values:

```cpp
#define ROCKY_LEFT_MOTOR_INVERT true
#define ROCKY_RIGHT_MOTOR_INVERT true
```

Only change the side that is reversed.

### MAX98357A-style I2S amplifier

The current pin reservation for a confirmed MAX98357A-compatible mono I2S amplifier is:

| Amplifier pin | XIAO |
|---|---|
| BCLK | GPIO3 / D2 |
| LRC/LRCK/WS | GPIO4 / D3 |
| DIN/DATA | GPIO2 / D1 |
| VIN | External regulated supply; 5 V recommended |
| GND | Common ground |
| SPK+ and SPK− | Directly to a 4 Ω or 8 Ω moving-coil speaker |

The amplifier has now been identified as **MAX98357A**. Check the breakout silkscreen before wiring optional `SD/MODE` or `GAIN` pins because module layouts differ.

For a MAX98357A-compatible board, the speaker connects between `SPK+` and `SPK−`. Neither speaker lead may be connected to GND because the output is bridge-tied. Leave `SD/MODE`, `GAIN`, and any `MCLK` pin exactly as the module documentation specifies; they are not assigned to spare XIAO GPIOs in this stage.

### Circuit diagram

The complete source diagram is [`complete_wiring.mmd`](complete_wiring.mmd), and the pin table is [`PIN_DIAGRAM.md`](PIN_DIAGRAM.md). Render the complete diagram with:

```text
manus-render-diagram xiao_bringup/complete_wiring.mmd xiao_bringup/complete_wiring.png
```

`wiring.mmd` and `wiring.png` remain as compatibility filenames and contain the same complete architecture.

For the physical build, use [`ASSEMBLY_LAYOUT.md`](ASSEMBLY_LAYOUT.md) and [`SIMPLE_ASSEMBLY_LAYOUT.png`](SIMPLE_ASSEMBLY_LAYOUT.png). This version minimizes wiring by placing the XIAO behind the OLED, keeping the DRV8833 between the motors at the rear, placing the MAX98357A beside the speaker, using one shared 5 V rail for the servo and amplifier, and using one common ground bus. The motor supply remains a separate VM rail.

The external signal pins are intentionally adjacent: D0 servo, D1-D3 MAX98357A, D4-D5 OLED, and D6-D9 DRV8833. The Sense microphone and camera remain on their internal GPIOs. See [`COMPACT_PIN_TABLES.md`](COMPACT_PIN_TABLES.md) for the one-table-per-part view.

The final architecture reserves these pins for later modules:

| Future module | Reserved GPIOs |
|---|---|
| Sense PDM microphone | GPIO42 clock, GPIO41 data |
| MAX98357A I2S audio | GPIO3 BCLK, GPIO4 LRCK, GPIO2 data |
| OLED | GPIO5 SDA, GPIO6 SCL |
| Servo | GPIO1 |
| Sense SD-CS | GPIO21; deliberately unused by this map |
| Sense camera | Keep the camera pin set untouched |

## Arduino IDE setup

1. Install Arduino IDE.
2. Add the Espressif package URL:

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

3. Install the Espressif `esp32` board package.
4. Select **XIAO_ESP32S3**.
5. In Tools, enable PSRAM when the option is available. The Sense camera needs PSRAM for reliable frame buffers.
6. Install these libraries:
   - Adafruit GFX Library
   - Adafruit SSD1306
   - ESP32Servo
7. Open `xiao_bringup.ino`; keep `xiao_config.h` in the same folder.
8. Select the XIAO USB port and upload.
9. Open Serial Monitor at `115200` baud with newline line ending.

If the port is missing, hold BOOT while connecting the data USB-C cable. If necessary, hold BOOT, tap RESET, release BOOT, and select the bootloader port.

## Wi-Fi configuration

The firmware stores credentials in ESP32 NVS Preferences instead of compiling them into the sketch.

If there are no saved credentials, it starts:

```text
SETUP AP: Rocky-XIAO-Setup IP: 192.168.4.1
```

Connect your phone or laptop to `Rocky-XIAO-Setup`, open `http://192.168.4.1`, choose **Configure Wi-Fi**, enter a 2.4 GHz SSID and password, and save. Reconnect to your normal Wi-Fi and open the IP printed in Serial Monitor.

The ESP32-S3 uses 2.4 GHz Wi-Fi. To move the robot between rooms or networks, use the dashboard’s Wi-Fi configuration page; no firmware change is required.

## Single-function test sequence

Start with only the OLED connected:

```text
test oled
face idle
face listening
face thinking
face working
face speaking
```

The important distinction is now:

- **Thinking:** alternating side glance, three-dot thought mouth, `thinking...` label.
- **Working:** narrower scanning eyes, two-line execution mouth, `executing...` label.

Then test the servo:

```text
test servo
pan 90
pan 60
pan 120
pan 0
pan 180
```

Use 0° and 180° only after checking the physical stop.

Then diagnose each DRV8833 channel independently with the robot lifted:

```text
motor speed 100
test left
test right
```

If both individual tests work, test the combined directions:

```text
motor forward
motor stop
motor back
motor stop
motor left
motor stop
motor right
motor stop
test motor
```

Start with speed 80–100. Increase only if the motor starts reliably and the driver remains cool. The firmware applies a 1.5-second motor timeout as a safety boundary.

If `test left` and `test right` work but `motor forward` makes one track oppose the other, the firmware defaults to a mirrored-track correction on the right channel. If your mechanical mounting is different, change only these flags in `xiao_config.h` and re-upload:

```cpp
#define ROCKY_LEFT_MOTOR_INVERT false
#define ROCKY_RIGHT_MOTOR_INVERT true
```

If an individual channel does not move, check that channel’s AIN/BIN pair, motor supply VM, common ground, nSLEEP/SLP HIGH, and the motor’s stall current. Do not debug this by increasing PWM above 100 until the wiring is proven.

The dashboard also provides runtime polarity buttons. These settings are stored in XIAO NVS and survive reboot:

```text
motor invert left on
motor invert left off
motor invert right on
motor invert right off
```

Use this calibration sequence with the robot lifted: set speed to 80–100, turn both inversion flags off, test forward briefly, then toggle only the side that runs backward. For a right turn, the left track should move forward and the right track backward. Stop after every short test.

Finally test Wi-Fi:

```text
test wifi
status
```

 The browser dashboard exposes the same controls as buttons, plus a live status panel and Wi-Fi configuration page.

### MAX98357A first audio test

The complete firmware initializes the MAX98357A on the adjacent audio pins: `DIN=GPIO2 / D1`, `BCLK=GPIO3 / D2`, and `LRC/LRCK=GPIO4 / D3`. It provides a deliberately low-volume square-wave tone test. This confirms the I2S wiring, amplifier power, and speaker output before adding any voice or TTS layer.

With the speaker connected only between `SPK+` and `SPK−`, run:

```text
status
test audio
audio tone 440 1500
audio stop
```

Start with low volume. If the tone is silent, check `VIN`, common GND, `BCLK`, `LRC/LRCK`, `DIN`, and both bridge-tied speaker wires. Do not connect either speaker output to GND.

## Head-state integration and dance

Head following is enabled by default. The servo moves to a small set of safe expressive poses when the face enters listening, thinking, working, curious, sad, or error states. It does not continuously chase the eyes, which avoids jitter.

```text
head auto
head off
dance start
dance stop
```

Dance is non-blocking and combines pan movement, happy/curious/surprised/laughing expressions, and short differential-track movements. Keep the robot lifted for the first run and use low motor speed.

## Microphone first test

The Sense microphone is on the expansion board; no loose microphone wire is required. The firmware uses the official PDM mapping:

| Signal | XIAO GPIO |
|---|---:|
| PDM clock | GPIO42 |
| PDM data | GPIO41 |

Run:

```text
test mic
```

Speak, clap, or tap near the microphone. Serial Monitor should print changing `MIC level` values. The dashboard status panel also reports `microphone` and `micLevel`. This is a level test only; it does not yet perform speech recognition or play audio.

## Personalities

```text
personality calm
personality rocky
personality engineer
personality spartan
personality isabella
```

**Rocky** replaces the old Musical label. It is a warm, curious, practical, observant problem-solver with understated humor and a scientist/explorer feel. The current bring-up expresses that through a restrained face accent and mode label; the spoken style will be added later.

**Isabella** is a soft animated AI-girl personality: gentle rounded eyes, bow/sparkle accents, warm expression, soft mouth animation, and subtle randomized mood changes between cheerful, curious, laughing, surprised, and calm. This is an original visual/personality direction, not a copy of a particular character or voice recording.

```text
personality isabella
```

The legacy commands `personality musical`, `personality jolly`, and `personality anime` remain accepted as aliases so old test notes do not break, but the dashboard and status now show `rocky` and `isabella`.

When the future audio backend is added, Isabella should use a soft, warm, expressive synthetic voice with gentle energy. Do not clone a real person’s voice without permission.

## Camera preview

The XIAO Sense camera is installed through the Sense camera connector; there are no loose camera wires to connect. Seat the camera/expansion board firmly, install the antenna, and keep the microSD card out during this first camera test. The camera occupies GPIO10, 11, 12, 13, 14, 15, 16, 17, 18, 38, 39, 40, 47, and 48 internally, so do not reuse them.

After uploading and joining Wi-Fi, run:

```text
test camera
```

Then open the dashboard. The Camera section displays a single JPEG snapshot. You can also open:

```text
http://XIAO_IP/camera.jpg
```

This stage supports snapshots and a low-rate live mode. Open `http://XIAO_IP/camera/live` or press **Live mode** in the dashboard. Live mode refreshes `/camera.jpg` approximately every 300 ms rather than holding a blocking MJPEG connection, so the OLED, servo, motors, and microphone remain responsive. If Serial Monitor says `CAMERA ERROR`, first confirm PSRAM is enabled, the camera board is seated correctly, and the selected board is `XIAO_ESP32S3`.

### Windows compile error involving `NetworkClient`

If Arduino reports:

```text
cannot bind non-const lvalue reference of type 'NetworkClient&'
to an rvalue of type 'WiFiClient'
```

use the updated `xiao_bringup.ino` from this package. The camera handler stores the returned client as a `WiFiClient` value so it works with ESP32 WebServer variants that return a temporary client object.

Also inspect the Arduino build output for the selected WebServer library. Prefer the WebServer library shipped with the Espressif ESP32 board package. If Arduino says it is using a separate user library such as:

```text
C:\Users\<you>\OneDrive ...\Documents\Arduino\libraries\WebServer
```

close Arduino IDE, move that user `WebServer` folder out of `Documents\Arduino\libraries`, reopen Arduino IDE, and compile again. The build should then report WebServer under your ESP32 package directory. The same applies to stray user `WiFi` libraries: the ESP32 package’s WiFi library should be selected for `XIAO_ESP32S3`.

## Dashboard URLs

After Wi-Fi connects:

```text
http://XIAO_IP/
http://XIAO_IP/api/status
http://XIAO_IP/wifi
```

The dashboard is intentionally local and unauthenticated in this bring-up stage. Do not expose it to the public internet.

## Next step after this stage

Once the camera snapshot works alongside the OLED, servo, DRV8833, and Wi-Fi, freeze this hardware base and move to audio in separate test sketches. The MAX98357A board is identified; check its breakout silkscreen before the first power test:

1. Test the Sense onboard PDM microphone alone.
2. Test the MAX98357A and speaker alone with `test audio`.
3. Add PCM playback and microphone capture as separate modules.
4. Test microphone capture plus speaker playback.
5. Add push-to-talk and listening/thinking/speaking states.
6. Add response text on the OLED.
7. Add the local modular LLM backend.
8. Add camera presence detection using snapshots, not continuous streaming.
9. Add wake-word detection and then combine camera/audio behavior.

This order keeps failures isolated and prevents the voice stack from hiding a power, pin, or motor problem.

## Knowledge-base references

- [Seeed XIAO ESP32-S3 getting started](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [Seeed XIAO ESP32-S3 Sense microphone](https://wiki.seeedstudio.com/xiao_esp32s3_sense_mic/)
- [TechTalkies XIAO project](https://github.com/TechTalkies/Xiaozhi-for-XiaoESP32S3)
- [TI DRV8833 product page](https://www.ti.com/product/DRV8833)
- [TI DRV8833 datasheet](https://www.ti.com/lit/gpn/DRV8833)

Every ZIP release must include the root `CHANGE_SUMMARY.md`.

## OTA and reusable reference integration

The XIAO firmware now includes an optional HTTPS manifest check and OTA update service. It reuses the existing `rocky-xiao` Wi-Fi preferences, so the dashboard Wi-Fi configuration remains the only provisioning path. The board checks the manifest once at boot and downloads only a newer semantic version.

For the full setup, read [`docs/xiao_ota_and_reference_integration.md`](../docs/xiao_ota_and_reference_integration.md). The short sequence is:

1. Upload the current sketch over USB and configure Wi-Fi from `Rocky-XIAO-Setup` if needed.
2. Push this project to GitHub and enable GitHub Pages with **GitHub Actions** as the source.
3. Copy `ota_target.example.h` to the ignored `ota_target.h` and replace its repository URLs.
4. Upload the manifest-enabled sketch once over USB.
5. Let GitHub Actions publish a newer version, then reboot the XIAO while it is stationary and watch the Serial Monitor for the manifest and update messages.

Keep the original USB upload path available as recovery. Do not expose the local dashboard directly to the public internet. The referenced desktop-pet OLED work is being used as the modular character/state model; the richer frame tables can be added later without coupling them to OTA.

The project root includes adapted CI/CD workflows under `.github/workflows/`. `build-firmware.yml` verifies the XIAO sketch, while `publish-ota-pages.yml` publishes the XIAO binary and manifest.
