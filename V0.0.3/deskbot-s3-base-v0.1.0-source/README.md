# DeskBot-S3 Base

A **standalone, modular foundation** for the Seeed Studio **XIAO ESP32-S3** and **XIAO ESP32-S3 Sense**. It builds a factory firmware image for browser flashing, provides local Wi-Fi provisioning, and makes it safe to test peripherals one at a time before adding the full DeskBot navigation stack.

> **Release:** `v0.1.0`  
> **Target:** XIAO ESP32-S3 / XIAO ESP32-S3 Sense, 8 MB flash  
> **Factory image:** `release/deskbot-s3-base-v0.1.0.factory.bin` (flash at `0x0`)

## What this first base release does

- Runs independently after USB is removed; no laptop is required after setup.
- Uses a **local captive portal** if saved Wi-Fi is unavailable. Join `DeskBot-S3-Setup` with password `deskbot-s3`, then visit `http://192.168.4.1`.
- Stores Wi-Fi credentials on the board so the bot reconnects after moving rooms or locations. The local dashboard can replace or clear them at any time.
- Serves a local admin page for device status, Wi-Fi setup, I²C scanning, Sense-camera snapshots, and **safe motor testing**.
- Keeps all motor outputs off at boot and stops them 500 ms after a test command, preventing an unattended drive command.
- Supports the official XIAO ESP32-S3 Sense OV2640 camera pin map when the camera module is present; the plain XIAO ESP32-S3 remains fully usable without it.
- Uses two OTA-size application partitions so later releases can add safe over-the-air update capability.

## Purposeful scope of v0.1.0

This is the **electrically conservative base**, not a claim that every optional DeskBot part is already present. The upstream DeskBot’s VL53L5CX, ISM330DHCX, and MMC5983MA integrations depended on an ESP32-WROOM wiring map. Here they are added in stages: first prove the I²C bus and motor driver, then add each sensor driver with its own test page. This avoids one unsupported part blocking the whole robot.

## Recommended low-cost hardware path

| Stage | Buy/connect | Why this choice |
|---|---|---|
| 1 | XIAO ESP32-S3 or XIAO ESP32-S3 Sense + known-good USB-C data cable | Enables all firmware, Wi-Fi, and web dashboard work immediately. |
| 2 | **TB6612FNG** dual H-bridge + two N20 gear motors | Prefer this over L298N for small robots: lower voltage loss, 3.3 V logic-friendly, compact, and efficient. Check each motor’s stall current; use a DRV8833/DRV8871 class driver if it exceeds the TB6612FNG channel rating. |
| 3 | VL53L5CX 8×8 ToF breakout | Adds the DeskBot-style obstacle map once the bus scanner sees it. |
| 4 | ISM330DHCX + MMC5983MA breakouts | Add only after the basic drive base is stable; they provide motion and heading data. |
| 5 | Sense camera only if needed | The Sense module makes visual tests easy; it is optional for motor/sensor development. |

## Connections

Use the following **default base mapping**. These values live in `include/DeskBotConfig.h`, so future versions can remap them without touching the network/dashboard code.

| XIAO label / GPIO | Connect to | Notes |
|---|---|---|
| D0 / GPIO 1 | TB6612FNG AIN1 | Left motor direction / PWM input |
| D1 / GPIO 2 | TB6612FNG AIN2 | Left motor direction / PWM input |
| D2 / GPIO 3 | TB6612FNG BIN1 | Right motor direction / PWM input |
| D3 / GPIO 4 | TB6612FNG BIN2 | Right motor direction / PWM input |
| D4 / GPIO 5 | I²C SDA | VL53L5CX, IMU, and magnetometer share this bus |
| D5 / GPIO 6 | I²C SCL | Use 3.3 V-compatible breakouts; most include pull-ups |
| D8 / GPIO 7 | VL53L5CX LPn/XSHUT (optional) | Allows controlled sensor enable/reset |
| 3V3 | Sensor VCC + TB6612FNG VCC | **Logic/sensor power only** — never motor power |
| GND | Driver GND, sensor GND, regulator GND | Every subsystem must have a common ground |
| Battery/regulator motor rail | TB6612FNG VM | Match the motor’s rated voltage; add a bulk capacitor at the driver |
| TB6612FNG AO1/AO2, BO1/BO2 | Left/right N20 motors | Swap a motor pair if its physical direction is backwards |

For a first motor-driver test, tie TB6612FNG `PWMA`, `PWMB`, and `STBY` high to its **3.3 V logic rail**. The firmware performs PWM on the AIN/BIN pins and holds all directions low until you arm test outputs in the local dashboard.

## Power and protection rules

1. **Never power motors from XIAO 3V3.** Use a dedicated motor supply attached to the driver’s `VM`.
2. Keep USB connected only while programming or bench testing. Disconnect the robot battery while USB is connected unless the supplies are deliberately ORed through correct power circuitry.
3. Add a **470–1000 µF electrolytic capacitor** across `VM` and GND near the motor driver plus a 0.1 µF ceramic decoupler. This protects the XIAO from motor-start voltage dips.
4. Use a single common ground. Do not connect a 5 V signal directly to a XIAO GPIO.
5. Test with wheels raised off the desk first; arm motors only after confirming driver wiring.
6. Keep I²C lines at 3.3 V. If a breakout has 5 V pull-ups, use a logic-level shifter or power the breakout correctly.

## Build and release commands

```bash
# Install PlatformIO once, then compile
pip install platformio
pio run

# Generate a single factory image at flash offset 0x0
python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 merge_bin \
  --flash_mode qio --flash_freq 80m --flash_size 8MB \
  -o release/deskbot-s3-base-v0.1.0.factory.bin \
  0x0 .pio/build/xiao_esp32s3/bootloader.bin \
  0x8000 .pio/build/xiao_esp32s3/partitions.bin \
  0x10000 .pio/build/xiao_esp32s3/firmware.bin
```

The project records additions and changes in [`CHANGELOG.md`](CHANGELOG.md). See [`docs/deskbot-s3-wiring.md`](docs/deskbot-s3-wiring.md) for the connection table and circuit diagram.
