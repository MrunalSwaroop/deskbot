# Rocky-Wall-E POC

Every ZIP release includes [`CHANGE_SUMMARY.md`](CHANGE_SUMMARY.md), which records the requirements addressed, errors corrected, files changed, and verification performed for that release. Update it before creating future ZIP packages.

## Current XIAO stage

`xiao_bringup/xiao_bringup.ino`, `xiao_bringup/xiao_config.h`, and `xiao_bringup/README.md` are now the active XIAO ESP32-S3 Sense firmware: OLED, pan servo, DRV8833/N90 motors, Wi-Fi provisioning, a local browser dashboard, separate thinking/working states, Rocky and Isabella personalities, camera snapshot/live preview, PDM microphone monitoring, head-state movement, dance, and an optional HTTPS manifest-based OTA update path.

This package is a staged proof of concept for the expressive tracked robot described in the supplied references. The XIAO is now the final controller for the bring-up; the earlier UNO work remains as historical reference and is not required for the XIAO build.

## Recommended architecture

Use the boards in three roles instead of trying to make both boards do everything at once.

| Stage | Controller | Responsibilities | What is proven |
|---|---|---|---|
| 1 | Arduino UNO R4 WiFi | OLED face, one pan servo, optional RGB status LED, L298D/L293D tracks, serial command protocol | The physical robot body and expressions |
| 2 | Generic ESP32 with USB-C | Wi-Fi HTTP gateway; forwards the same commands to the UNO | Remote commands and future voice/AI integration |
| 3 | XIAO ESP32S3 Sense | Camera, digital microphone, Wi-Fi, OLED, audio roadmap, motion commands, local dashboard, optional OTA | The integrated AI companion |

The important design decision is the **line-based command protocol**. The UNO accepts commands such as `EYE listen`, `MOVE forward`, and `SERVO pan 70`. The generic ESP32 and the future XIAO can both send those same commands. This prevents the face and motor code from being tied to one particular network or AI firmware.

The XIAO ESP32S3 Sense is the right final controller because it integrates an OV3660 or OV2640 camera variant, a digital microphone, Wi-Fi/BLE, 8 MB PSRAM, 8 MB flash, and battery support.[1] The current generic ESP32 may not have the camera, microphone, PSRAM, or compatible pinout required by the final Xiaozhi build. Treat it as a gateway and experiment board, not as a drop-in replacement for the XIAO.

## Files in this package

| File | Purpose |
|---|---|
| `eyes_only_test/eyes_only_test.ino` | Safe first test for the OLED face, mouth, and expressions; no servos or motors are used |
| `eyes_only_test/eyes.h` | Eye bitmaps used by the eyes-only test |
| `face_neck_test.ino` | Full face plus conservative pan/tilt servo test; motors are not used |
| `eyes.h` | Eye bitmaps beside `face_neck_test.ino` for Arduino IDE compilation |
| `uno_wifi_face_server/uno_wifi_face_server.ino` | UNO R4 WiFi face server with one-axis pan support |
| `uno_wifi_face_server/arduino_secrets.h.example` | Private Wi-Fi credential template |
| `uno_wifi_face_server/eyes.h` | Eye bitmaps for the Wi-Fi face server |
| `uno_r4_current_desk_buddy/uno_r4_current_desk_buddy.ino` | Non-destructive full desk-buddy firmware using the normal RA4M1 + ESP32 bridge + WiFiS3 architecture, with clean face-first OLED, 0–180° pan, RGB state LED, and embedded browser dashboard |
| `uno_r4_current_desk_buddy/arduino_secrets.h.example` | Private Wi-Fi and optional direct LLM credential template |
| `uno_r4_current_desk_buddy/eyes.h` | Eye bitmaps for the current-architecture desk-buddy firmware |
| `uno_r4_face_motion/uno_r4_face_motion.ino` | First-stage UNO firmware for OLED eyes, servos, tracks, and serial commands |
| `uno_r4_face_motion/eyes.h` | Eye bitmaps copied from the referenced RobotEyes project |
| `esp32_gateway/esp32_gateway.ino` | Generic ESP32 Wi-Fi and USB command gateway |
| `llm_bridge/robot_llm_bridge.py` | Laptop-side natural-language-to-safe-command bridge |
| `integration_relay/relay_server.py` | Prototype GitHub, Manus, Alexa, and notification relay scaffold |
| `integration_relay/.env.example` | Environment template for relay secrets; contains no real key |
| `.gitignore` | Excludes local Arduino and relay credentials |
| `tools/poc_serial_console.py` | Optional computer serial console for testing without voice hardware |
| `tools/laptop_audio_bridge.py` | Temporary laptop microphone, text, and TTS helper; laptop does not call the LLM |
| `docs/voice_personality.md` | Voice and personality direction for the future AI layer |
| `docs/neck_test.md` | Servo wiring, limits, and test procedure |
| `docs/wifi_llm_stage.md` | UNO R4 WiFi and local LLM setup guide |
| `docs/non_destructive_desk_buddy.md` | Current-architecture states, dance, direct LLM, and temporary audio guide |
| `xiao_bringup/xiao_bringup.ino` | Modular XIAO bring-up with OLED, servo, DRV8833, Wi-Fi, dashboard, and personalities |
| `xiao_bringup/xiao_config.h` | Central XIAO pin map, future peripheral reservations, limits, and motor inversion flags |
| `xiao_bringup/device_config.h`, `ota_service.h`, `ota_pages_root_ca.h` | Complete firmware support headers for repository-neutral OTA and version defaults |
| `xiao_bringup/README.md` | XIAO DRV8833 wiring, circuit notes, one-function tests, and next-stage roadmap |
| `xiao_bringup/PIN_DIAGRAM.md` | Complete XIAO/OLED/DRV8833/servo/audio pin table and checklist |
| `xiao_bringup/COMPACT_PIN_TABLES.md` | Compact table-format connections for every part using adjacent XIAO headers |
| `xiao_bringup/complete_wiring.mmd` and `.png` | Complete editable and rendered XIAO wiring diagram |
| `xiao_bringup/ASSEMBLY_LAYOUT.md` | Simplified physical placement and point-to-point wiring checklist |
| `xiao_bringup/SIMPLE_ASSEMBLY_LAYOUT.mmd` and `.png` | Simplified physical assembly layout source and image |
| `xiao_bringup/wiring.mmd` and `.png` | Compatibility copies of the complete wiring diagram |
| `xiao_bringup/ota_service.cpp` and `xiao_bringup/ota_service.h` | Adapted HTTPS manifest and ESP32 OTA update service |
| `xiao_bringup/ota_target.example.h` | Local OTA target template; copy to ignored `ota_target.h` when publishing |
| `.github/workflows/build-firmware.yml` | GitHub Actions compile check for the XIAO sketch |
| `.github/workflows/publish-ota-pages.yml` | GitHub Pages firmware and manifest publisher |
| `docs/xiao_ota_and_reference_integration.md` | OTA bootstrap, recovery, dashboard, and referenced-task integration guide |
| `docs/referenced_tasks/KNOWLEDGE_BASE.md` | Persisted notes from the referenced OTA and desktop-pet tasks |
| `scripts/bin2ota.py` | Referenced OTA packaging helper retained for future release tooling |

## First test: face only

Begin with `eyes_only_test/eyes_only_test.ino`, not the full motor firmware. This sketch uses only the UNO R4, the OLED, and the USB serial connection. It does not attach servos or drive motors, so it is the safest way to validate the display. It includes the RobotEyes eye sprites plus original drawn eyebrows, cheeks, mouth shapes, sleeping lines, and a speaking mouth animation.

### Wiring

| OLED pin | UNO R4 pin |
|---|---|
| GND | GND |
| VCC | 3.3 V or 5 V according to the OLED module marking |
| SDA | SDA or A4 |
| SCL | SCL or A5 |

The sketch assumes a **128×64 SSD1306 I2C OLED at address `0x3C`**. If the display is blank, run an I2C scanner and change `OLED_ADDRESS` to `0x3D` if that is the detected address. A 128×32 display will not render these 32×32 eye sprites correctly.

### Upload

1. Install **Adafruit GFX Library** and **Adafruit SSD1306** in Arduino IDE.
2. Create or open the folder `eyes_only_test`.
3. Put `eyes_only_test.ino` and `eyes.h` in that same folder.
4. Open the `.ino` file in Arduino IDE.
5. Select **Arduino UNO R4 WiFi** under **Tools → Board**.
6. Select the UNO's USB port under **Tools → Port**.
7. Click **Upload**.
8. Open **Serial Monitor** and set the baud rate to **115200**.
9. Set the line ending to **Newline** or **Both NL & CR**.

The OLED first shows `HELLO, HUMAN`, `ROCKY READY`, and `I AM CALM`, then displays the idle face. The Serial Monitor should print `Rocky full face test ready.`

### Commands

Type one command and press Enter. You may use either the short form or the future-compatible form:

| Type this | OLED result |
|---|---|
| `happy` or `EYE happy` | Smile, raised brows, and small cheek marks |
| `listen` or `EYE listen` | Attentive face and small listening mouth |
| `speak` or `EYE speak` | Animated open/closed speaking mouth |
| `sleep` or `EYE sleep` | Closed eyes and quiet sleeping mouth |
| `surprise` or `EYE surprise` | Wide eyes and round surprised mouth |
| `sad` or `EYE sad` | Downturned brows and mouth |
| `curious` or `EYE curious` | Slightly inquisitive face |
| `worried` or `EYE worried` | Concerned brows and mouth |
| `left` or `EYE left` | Eyes look left |
| `right` or `EYE right` | Eyes look right |
| `idle` or `EYE idle` | Neutral face |
| `personality off` | Freezes the selected expression |
| `personality on` | Enables blinking, speaking-mouth animation, and idle glances |
| `mouth smile` / `mouth open` / `mouth flat` | Redraws the current face for mouth inspection |
| `clear` | Clears the OLED |
| `HELP` | Prints the command list |

For the exact test you described, type `happy` in Serial Monitor and press Enter. You should receive `OK showing happy`, and the OLED should change immediately. If it does not, check the Serial Monitor line ending, the I2C address, and the SDA/SCL wiring before moving on.

The emotional direction is a **small, caring desk companion**: calm, attentive, reassuring, and occasionally curious. It uses simple visual communication rather than attempting to copy a particular movie character. The face should feel helpful and gentle while remaining an original Rocky-Wall-E design.

## What to buy or identify before wiring

You can start Stage 1 with the boards and OLED you already have. The following details must be confirmed before connecting the generic ESP32 to anything:

1. Identify the exact generic ESP32 board and its printed pin labels. A USB-C connector alone does not identify the module or its safe pins.
2. Identify whether the OLED is a 128×64 SSD1306 I2C module at address `0x3C`. The firmware assumes that configuration.
3. Obtain two micro servos for the pan/tilt neck.
4. Obtain an L298N motor driver, two gear motors, tracks, and a motor battery if the mobile body is part of the first POC.
5. Obtain a regulated 5 V supply or battery arrangement suitable for the servo and audio loads. Do not power the motors or servos from an Arduino GPIO pin or from the ESP32 3.3 V rail.
6. Add a bidirectional logic-level converter, or use a carefully designed level-shifting circuit, before connecting UNO R4 UART signals to ESP32 UART signals. The UNO R4 RA4M1 side is 5 V logic, while ESP32 GPIO is 3.3 V logic. Never connect a UNO 5 V TX signal directly to an ESP32 RX GPIO.

## Stage 1: get the face working on the UNO R4

### Install Arduino libraries

In Arduino IDE, install these libraries from the Library Manager:

- Adafruit GFX Library
- Adafruit SSD1306
- Servo, if the IDE does not already provide it for the UNO R4

Open `uno_r4_face_motion/uno_r4_face_motion.ino`. Keep `eyes.h` in the same sketch folder. Select **Arduino UNO R4 WiFi** as the board and upload.

### OLED wiring

The RobotEyes firmware assumes a standard I2C OLED.

| OLED pin | UNO R4 connection | Note |
|---|---|---|
| SDA | A4 / SDA | Use the board's SDA pin if your revision labels it separately |
| SCL | A5 / SCL | Use the board's SCL pin if your revision labels it separately |
| GND | GND | Common ground |
| VCC | 3.3 V or 5 V according to the OLED module marking | Prefer 3.3 V when the module supports it; check I2C level compatibility |

After upload, open Serial Monitor at 115200 baud. You should see `OK ROCKY_WALLE_UNO_READY` and a pair of animated eyes.

### First commands

Send each command separately in the Serial Monitor. Set the line ending to **Newline** or **Both NL & CR**.

```text
HELP
EYE happy
EYE listen
EYE speak
EYE sleep
EYE surprise
EYE left
EYE right
SERVO pan 70
SERVO tilt 110
```

If the OLED remains blank, run an I2C scanner first. The most common causes are the wrong address (`0x3D` instead of `0x3C`), swapped SDA/SCL, missing ground, or a display that is not SSD1306-compatible.

## Stage 1b: add the servos and tracks

The supplied firmware uses these UNO pins:

| Function | UNO pin |
|---|---:|
| Pan servo signal | D3 |
| Tilt servo signal | D4 |
| Left motor enable/PWM | D5 |
| Left motor input A/B | D6 / D7 |
| Right motor enable/PWM | D10 |
| Right motor input A/B | D11 / D12 |

Connect servo grounds and motor-driver grounds to the UNO ground. Power the servos and motors from their own suitable supply. Tie the external supply ground to UNO ground so the signal reference is shared.

For the L298N, remove or disable the ENA/ENB jumpers if you want PWM speed control. Connect the two enable inputs to D5 and D10. Connect the four direction inputs to D6, D7, D11, and D12. Keep the motor battery wiring away from the OLED and signal wiring. Add a physical power switch and test with the wheels lifted off the table.

The motor direction may be reversed because motor gearboxes are mounted as mirror images. If `MOVE forward` makes one track rotate backward, swap that motor's two output wires or invert the corresponding direction logic in `motor()`.

## Stage 2: add the generic ESP32 as a command gateway

Do not begin with the UART connection. First upload `esp32_gateway/esp32_gateway.ino` to the generic ESP32 and test it over USB. Change these values in the file:

```cpp
const char *WIFI_SSID = "REPLACE_WITH_WIFI_NAME";
const char *WIFI_PASSWORD = "REPLACE_WITH_WIFI_PASSWORD";
const int ESP32_RX_FROM_UNO = 16;
const int ESP32_TX_TO_UNO = 17;
```

GPIO16 and GPIO17 are common on many ESP32-WROOM development boards, but you must verify the pinout of your exact board before using them. Keep the ESP32 disconnected from the UNO UART until the pinout and voltage levels are confirmed.

After upload, open the ESP32 serial monitor at 115200. It will print an IP address when it joins Wi-Fi. With the ESP32 and computer on the same network, test:

```text
http://ESP32_IP/health
http://ESP32_IP/cmd?x=EYE%20happy
http://ESP32_IP/cmd?x=SERVO%20pan%2070
http://ESP32_IP/cmd?x=MOVE%20stop
```

For the first safe test, connect only the ESP32 USB serial and use the `/health` endpoint. Then add the UART link through the level shifter. The UART link is:

| Signal | Direction | Required treatment |
|---|---|---|
| UNO TX / D1 | UNO → ESP32 RX | Shift 5 V down to 3.3 V |
| ESP32 TX | ESP32 → UNO RX / D0 | Use a level shifter or confirm the UNO input tolerance; a shifter is preferred |
| GND | Common | Required |

The UNO R4 also contains a separate ESP32-S3 Wi-Fi/Bluetooth module, but this POC does not use that module. The generic ESP32 board is being used as a simple, replaceable network gateway.[2]

## Stage 3: migrate to the XIAO and Xiaozhi

When the XIAO arrives, do not port the generic gateway code line by line. Flash and test the referenced Xiaozhi project first. That project expects a XIAO ESP32-S3, a 128×64 I2C OLED, an I2S MEMS microphone, a MAX98357A I2S amplifier, and a 4 Ω speaker.[3]

The referenced Xiaozhi wiring is:

| Device | XIAO GPIO |
|---|---:|
| OLED SDA | GPIO5 / D4 |
| OLED SCL | GPIO6 / D5 |
| Microphone BCLK/SCK | GPIO44 / D7 |
| Microphone WS/LRCK | GPIO9 / D10 |
| Microphone SD/DOUT | GPIO1 / D0 |
| MAX98357A BCLK | GPIO7 / D8 |
| MAX98357A LRC/LRCK | GPIO4 / D3 |
| MAX98357A DIN | GPIO2 / D1 |

The XIAO pin mapping is not the same as the UNO mapping. Keep the motor and servo commands at the protocol level, then implement the output side in the XIAO firmware or use the UNO as a dedicated actuator controller.

The move to the XIAO ESP32S3 Sense is not necessarily a literal drop-in flash. The Sense board has an integrated digital microphone and camera, while the referenced Xiaozhi project documents an external I2S MEMS microphone and does not by itself complete the camera integration. For the first XIAO bring-up, follow the repository's documented external microphone and MAX98357A wiring if you have those parts. Then adapt the board configuration for the Sense microphone and add the camera path separately. Validate audio first, then camera, then motion.

A practical final architecture is:

```text
XIAO ESP32S3 Sense
  camera + microphone + Wi-Fi + Xiaozhi + personality
                │
                │  EYE / MOVE / SERVO commands
                ▼
UNO R4 WiFi
  OLED + servos + L298N + tracks
```

The XIAO can also drive the OLED and audio hardware directly, but retaining the UNO as a motor/servo controller reduces GPIO conflicts and keeps the mechanical subsystem isolated while you are iterating.

## POC definition of done

The POC is complete when all of the following work without changing the command vocabulary:

1. The UNO boots and displays idle eyes.
2. `EYE listen`, `EYE speak`, `EYE happy`, and `EYE sleep` produce visibly different expressions.
3. `SERVO pan` and `SERVO tilt` move the neck and stop at mechanically safe limits.
4. `MOVE forward`, `MOVE back`, `MOVE left`, `MOVE right`, and `MOVE stop` control both tracks.
5. The generic ESP32 returns a health response over Wi-Fi.
6. The generic ESP32 forwards an HTTP command to the UNO.
7. A computer can act as a temporary voice/AI client and send the same commands.
8. After the XIAO arrives, Xiaozhi can be brought up independently before motion control is attached.

## Recommended build order

Start with the OLED alone. Then add one servo. Then add the second servo. Then test the L298N with the tracks lifted. Only after each subsystem works independently should you connect the external motor battery.

The first AI demo should be a **push-to-talk desk robot**, not a fully autonomous roaming robot. The computer or XIAO can listen and speak, while the UNO displays `listen` and `speak` faces and performs simple motion commands. Add camera vision and autonomous motion only after the emergency stop and power system are reliable.

## Safety and reliability notes

The L298N, motors, and servos create voltage dips and electrical noise. Keep their supply separate from the logic rail. Add bulk capacitance near the motor-driver supply and keep all grounds connected at a deliberate common point.

Never test a tracked robot on the floor with an unverified `forward` command. Lift the tracks first. Add a hardware power switch that removes motor power while leaving the controller powered.

Do not expose the ESP32 HTTP endpoint to the public internet. Use it only on your private local network during the POC.

## References

[1]: https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html "Seeed Studio XIAO ESP32-S3 Sense product page"
[2]: https://docs.arduino.cc/hardware/uno-r4-wifi "Arduino UNO R4 WiFi official hardware page"
[3]: https://github.com/TechTalkies/Xiaozhi-for-XiaoESP32S3 "TechTalkies Xiaozhi AI Desk Buddy for XIAO ESP32-S3"
[4]: https://github.com/Picaio/roboteyes "Picaio RobotEyes Arduino OLED animation project"
[5]: https://www.huyvector.org/robots-kinetic/wall-e-ai "Huy Vector Wall-E AI build"
