# Rocky XIAO ESP32-S3 Sense — Compact Connection Tables

This is the simplified table view of the active hardware map. All external signal connections now use the adjacent XIAO header block **D0 through D9**. The Sense camera and PDM microphone remain on their internal Sense-board connections.

## XIAO external signal map

| XIAO pin | GPIO | Connected function |
|---|---:|---|
| D0 | GPIO1 | Pan servo signal |
| D1 | GPIO2 | MAX98357A `DIN / DATA` |
| D2 | GPIO3 | MAX98357A `BCLK` |
| D3 | GPIO4 | MAX98357A `LRC / LRCK / WS` |
| D4 / SDA | GPIO5 | OLED `SDA` |
| D5 / SCL | GPIO6 | OLED `SCL` |
| D6 / TX | GPIO43 | DRV8833 `AIN1` |
| D7 / RX | GPIO44 | DRV8833 `AIN2` |
| D8 | GPIO7 | DRV8833 `BIN1` |
| D9 | GPIO8 | DRV8833 `BIN2` |
| D10 | GPIO9 | Spare external GPIO; do not use the SD bus in this build |
| 3V3 | — | OLED VCC and DRV8833 nSLEEP/SLP HIGH |
| GND | — | Common ground bus |
| 5V / VBUS | — | XIAO board power reference; not a GPIO |

## SSD1306 OLED

| OLED | XIAO or supply |
|---|---|
| VCC | 3V3 |
| GND | GND / common ground |
| SDA | GPIO5 / D4 / SDA |
| SCL | GPIO6 / D5 / SCL |

The firmware uses I2C address `0x3C`. The alternate supported address is `0x3D`.

## Pan servo

| Servo wire | XIAO or supply |
|---|---|
| Signal | GPIO1 / D0 |
| Red / positive | External regulated 5 V rail |
| Brown/black / ground | Common ground |

Do not power the servo from XIAO 3V3.

## DRV8833 two-channel driver

| DRV8833 | XIAO or supply |
|---|---|
| AIN1 | GPIO43 / D6 / TX |
| AIN2 | GPIO44 / D7 / RX |
| BIN1 | GPIO7 / D8 |
| BIN2 | GPIO8 / D9 |
| nSLEEP / SLP | 3V3 |
| GND | Common ground |
| VM | Separate motor-supply positive |
| AOUT1 / AOUT2 | Left N90 motor wires |
| BOUT1 / BOUT2 | Right N90 motor wires |
| nFAULT | Leave unconnected for this stage |
| VCC / VLOGIC, if exposed | Follow the exact carrier-board documentation; do not guess |

## Left and right N90 motors

| Motor | DRV8833 connection |
|---|---|
| Left N90 wire 1 | AOUT1 |
| Left N90 wire 2 | AOUT2 |
| Right N90 wire 1 | BOUT1 |
| Right N90 wire 2 | BOUT2 |

If a track turns backward, stop the motor and change the persisted motor inversion setting rather than rewiring first.

## MAX98357A I2S mono amplifier

| MAX98357A | XIAO or supply |
|---|---|
| BCLK | GPIO3 / D2 |
| LRC / LRCK / WS | GPIO4 / D3 |
| DIN / DATA | GPIO2 / D1 |
| VIN | External regulated 5 V recommended |
| GND | Common ground |
| SPK+ | Speaker positive |
| SPK− | Speaker negative |
| SD / MODE | Leave as the module documentation specifies |
| GAIN | Leave as the module documentation specifies |
| MCLK | Leave disconnected if exposed |

Use a moving-coil speaker rated at 4 Ω or higher. Connect the speaker only between `SPK+` and `SPK−`. Do not connect either speaker output to GND.

## Sense onboard PDM microphone

No loose wires are required. These pins are internal to the Sense expansion board.

| Sense microphone signal | Internal XIAO GPIO |
|---|---:|
| PDM clock | GPIO42 |
| PDM data | GPIO41 |

## Sense camera

No loose wires are required. Keep these internal camera pins reserved:

| Sense camera function | Internal GPIOs |
|---|---|
| Camera DVP and control bus | GPIO10, 11, 12, 13, 14, 15, 16, 17, 18, 38, 39, 40, 47, 48 |

## Power rails

| Supply item | Positive connection | Negative connection |
|---|---|---|
| XIAO during development | USB-C | XIAO internal ground |
| OLED | XIAO 3V3 | XIAO GND |
| Servo | External regulated 5 V | Common ground |
| MAX98357A | External regulated 5 V | Common ground |
| DRV8833 motor power | Separate motor supply to VM | Common ground |
| N90 motors | Driver outputs only | Never connect motor wires directly to ground |

The simplest safe arrangement is one regulated 5 V rail for the servo and MAX98357A, one separate motor VM rail, and one common ground bus. If the servo causes audio noise or a controller reset, use separate regulated 5 V branches while keeping the grounds common.

## Pin adjacency answer

Yes. The active firmware now uses the adjacent external signal sequence:

```text
D0  servo
D1  amplifier DIN
D2  amplifier BCLK
D3  amplifier LRC/LRCK
D4  OLED SDA
D5  OLED SCL
D6  DRV8833 AIN1
D7  DRV8833 AIN2
D8  DRV8833 BIN1
D9  DRV8833 BIN2
```

This is the most practical contiguous arrangement because the Sense camera and PDM microphone occupy internal GPIOs, while the current motor/audio functions need ten external signal pins. The firmware configuration and diagrams have been updated to match this table.

## References

[1]: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/ "Seeed Studio XIAO ESP32-S3 Getting Started and Pin Map"
[2]: https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts "Adafruit MAX98357 I2S Class-D Mono Amplifier Pinouts"
[3]: https://www.ti.com/lit/gpn/drv8833 "Texas Instruments DRV8833 Dual H-Bridge Motor Driver Datasheet"
