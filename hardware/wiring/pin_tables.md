# Rocky XIAO ESP32-S3 Sense — Complete Pin Diagram

This document is the wiring reference for the current desk-robot bring-up. The active controller is the **Seeed XIAO ESP32-S3 Sense**. The editable structural diagram is [`complete_wiring.mmd`](complete_wiring.mmd), and the rendered image is [`complete_wiring.png`](complete_wiring.png).

> **Amplifier identified:** the user has confirmed the board is a **MAX98357A I2S mono amplifier**. The wiring below assumes the usual module labels `VIN`, `GND`, `BCLK`, `LRC/LRCK`, `DIN`, and `SPK+ / SPK−`. Confirm the exact breakout silkscreen before connecting optional `SD/MODE` or `GAIN` pins because carrier boards may expose them differently.

## 1. XIAO header assignments

The table uses both the XIAO header label and the ESP32-S3 GPIO number. The firmware configuration uses GPIO numbers. The XIAO pin names and Sense-board internal camera, PDM microphone, and SD reservations follow Seeed’s pin map.[1]

| XIAO label | ESP32-S3 GPIO | Current assignment | Direction or supply |
|---|---:|---|---|
| D0 | GPIO1 | Pan-servo signal | 3.3 V logic output to servo signal |
| D1 | GPIO2 | MAX98357A `DIN` | I2S data output; reserved for audio playback |
| D2 | GPIO3 | MAX98357A `BCLK` | I2S bit-clock output; reserved for audio playback |
| D3 | GPIO4 | MAX98357A `LRC/LRCK` | I2S word-select output; reserved for audio playback |
| D4 / SDA | GPIO5 | OLED `SDA` | I2C data |
| D5 / SCL | GPIO6 | OLED `SCL` | I2C clock |
| D6 / TX | GPIO43 | DRV8833 `AIN1` | PWM/direction output |
| D7 / RX | GPIO44 | DRV8833 `AIN2` | PWM/direction output |
| D8 | GPIO7 | DRV8833 `BIN1` | PWM/direction output; conflicts with Sense SD SCK if SD is used |
| D9 | GPIO8 | DRV8833 `BIN2` | PWM/direction output; conflicts with Sense SD MISO if SD is used |
| D10 | GPIO9 | Unassigned in this build | Do not use for SD MOSI while preserving the current map |
| — | GPIO21 | Sense SD-card CS reservation | SD deliberately unused in this build |
| — | GPIO41 | Sense PDM microphone data | Internal Sense connection |
| — | GPIO42 | Sense PDM microphone clock | Internal Sense connection |
| — | GPIO10, 11, 12, 13, 14, 15, 16, 17, 18, 38, 39, 40, 47, 48 | Sense camera | Internal Sense camera connection; do not reuse |
| 3V3 | — | OLED supply and DRV8833 `nSLEEP/SLP` HIGH reference | 3.3 V logic supply only |
| 5V / VBUS | — | Board power reference | USB-derived or correctly protected external input; not a GPIO |
| GND | — | Common electrical ground | Shared with servo, motor supply, and audio supply negatives |

## 2. OLED wiring

Use a 4-pin SSD1306 I2C OLED. The current firmware expects address `0x3C`; `0x3D` is the alternate address supported by the configuration file.

| OLED pin | Connect to XIAO | Notes |
|---|---|---|
| `GND` | `GND` | Common ground |
| `VCC` | `3V3` | Do not feed the OLED from an unknown 5 V rail unless the module explicitly supports it |
| `SDA` | `GPIO5 / D4 / SDA` | I2C data |
| `SCL` | `GPIO6 / D5 / SCL` | I2C clock |

Do not connect the OLED `SDA` or `SCL` wires to the motor driver. Keep the I2C wires short during the bring-up.

## 3. DRV8833 two-channel motor-driver wiring

The DRV8833 is a dual H-bridge. Channel A drives the left N90 motor, and channel B drives the right N90 motor. This firmware applies PWM directly to the four input pins, so there is no separate L293D-style enable wire.

| DRV8833 carrier label | Connect to | Purpose |
|---|---|---|
| `AIN1` | XIAO `GPIO43 / D6/TX` | Left motor input 1 |
| `AIN2` | XIAO `GPIO44 / D7/RX` | Left motor input 2 |
| `BIN1` | XIAO `GPIO7 / D8` | Right motor input 1 |
| `BIN2` | XIAO `GPIO8 / D9` | Right motor input 2 |
| `nSLEEP`, `SLP`, or `SLEEP` | XIAO `3V3` | Must be HIGH for the current firmware |
| `GND` | Common ground | Connect to XIAO GND and both external-supply negatives |
| `VM` | Separate motor supply positive | Motor voltage; select it for the N90 motors and driver carrier |
| `AOUT1`, `AOUT2` | Two wires of left N90 motor | Do not connect either motor lead to ground |
| `BOUT1`, `BOUT2` | Two wires of right N90 motor | Do not connect either motor lead to ground |
| `nFAULT` | Leave unconnected for now | Optional future diagnostic input |
| `VCC` or logic-supply pin, if present | Follow the exact carrier-board documentation | Carrier boards differ; do not assume every breakout has the same supply arrangement |

The motor supply must not be connected to the XIAO `3V3` pin. Add a physical motor-power switch. Keep the robot lifted for initial tests. A bulk capacitor close to the driver’s `VM` and `GND` is recommended according to the carrier-board documentation.

If one track runs backward, first stop the motors and use the persisted runtime calibration commands:

```text
test left
test right
motor invert left on
motor invert left off
motor invert right on
motor invert right off
```

The compile-time defaults are `left invert = false` and `right invert = true`, which normally suits mirrored track assemblies.

## 4. Pan-servo wiring

| Servo wire | Connect to | Notes |
|---|---|---|
| Signal, usually yellow/orange/white | XIAO `GPIO1 / D0` | Logic signal only |
| Positive, usually red | External regulated `+5 V` | Do not power the servo from XIAO `3V3` |
| Ground, usually brown/black | External supply `GND` | Must be tied to XIAO GND |

The firmware supports the requested `0–180°` command range. Test the mechanical limits carefully because the linkage may not safely reach both endpoints.

## 5. MAX98357A I2S amplifier wiring

The current firmware reserves the following I2S pins for a future audio playback test. The present bring-up monitors the Sense PDM microphone but does not yet play audio.

| Amplifier pin | Connect to | Notes |
|---|---|---|
| `BCLK` or `BCLK/CLK` | XIAO `GPIO3 / D2` | I2S bit clock |
| `LRC`, `LRCK`, or `WS` | XIAO `GPIO4 / D3` | I2S left/right clock |
| `DIN` or `DATA` | XIAO `GPIO2 / D1` | I2S audio data |
| `VIN` | External regulated `5 V` recommended | MAX98357A amplifier supply; use the exact breakout documentation for its allowable range |
| `GND` | Common ground | Connect to XIAO GND and external-supply negative |
| `SPK+` or `OUT+` | Speaker positive terminal | Bridge-tied output |
| `SPK−` or `OUT−` | Speaker negative terminal | Bridge-tied output |
| `SD` / `MODE` | Leave as the module documentation specifies | Do not assign a XIAO GPIO in this stage |
| `GAIN` | Leave as the module documentation specifies | Do not assign a XIAO GPIO in this stage |
| `MCLK` | Leave disconnected if exposed | MAX98357A operation does not require MCLK |

Use a moving-coil speaker rated at **4 Ω or higher**. The speaker must connect directly between `SPK+` and `SPK−`. **Neither speaker wire may be connected to GND**, because the output is bridge-tied.

## 6. Sense microphone and camera

These are internal connections on the XIAO ESP32-S3 Sense expansion board. No loose wires are required.

| Internal module | GPIO reservation | Current status |
|---|---|---|
| PDM microphone clock | GPIO42 | Firmware microphone-level test |
| PDM microphone data | GPIO41 | Firmware microphone-level test |
| Camera | GPIO10, 11, 12, 13, 14, 15, 16, 17, 18, 38, 39, 40, 47, 48 | Firmware snapshot and low-rate live preview |
| SD-card CS | GPIO21 | Reserved but unused |
| SD-card SCK/MISO/MOSI | GPIO7 / GPIO8 / GPIO9 | Not available in the current map because GPIO7 and GPIO8 are used by DRV8833; GPIO9 remains spare |

Do not reuse camera GPIOs for external accessories. Keep PSRAM enabled for reliable camera operation.

## 7. Power and common-ground checklist

The project uses separate power domains because the servo, motors, and audio amplifier can create current spikes.

1. Power the XIAO from USB-C during bring-up.
2. Power the OLED from XIAO `3V3` and `GND`.
3. Power the servo from an external regulated 5 V supply.
4. Power the DRV8833 `VM` from a separate motor supply suitable for the N90 motors.
5. Power the confirmed MAX98357A amplifier from a suitable external audio supply, preferably regulated 5 V for the first test.
6. Connect XIAO GND, servo-supply GND, motor-supply GND, and audio-supply GND together.
7. Never connect motor `VM` or servo `+5 V` to XIAO `3V3`.
8. Never feed 5 V into a XIAO GPIO.
9. Never connect either amplifier speaker output to GND.
10. Use a physical motor switch and test with the robot lifted.

## 8. Recommended connection and test order

Connect and test one electrical function at a time:

```text
test oled
face happy
test servo
pan 90
test left
test right
motor stop
test mic
test camera
```

After checking the breakout silkscreen and speaker wiring, connect the audio branch. The next audio stage should be a standalone low-volume I2S tone test. Do not begin microphone-to-speaker looping until the amplifier, speaker, power rail, and I2S pin assignment have passed that isolated test.

## References

[1]: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/ "Seeed Studio XIAO ESP32-S3 Getting Started and Pin Map"
[2]: https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts "Adafruit MAX98357 I2S Class-D Mono Amplifier Pinouts"
[3]: https://www.ti.com/lit/gpn/drv8833 "Texas Instruments DRV8833 Dual H-Bridge Motor Driver Datasheet"
