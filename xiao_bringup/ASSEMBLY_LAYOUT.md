# Simplified Rocky Desk Robot Assembly Layout

This is the **physical assembly version** of the pin diagram. It is intended to answer where each module should sit and exactly which wires should be run. The front of the robot is the OLED face. The XIAO sits immediately behind it. The DRV8833 and power connections stay at the rear, while the amplifier and speaker stay on the right side.

The rendered layout is [`SIMPLE_ASSEMBLY_LAYOUT.png`](SIMPLE_ASSEMBLY_LAYOUT.png). The editable source is [`SIMPLE_ASSEMBLY_LAYOUT.mmd`](SIMPLE_ASSEMBLY_LAYOUT.mmd). The full electrical pin table remains in [`PIN_DIAGRAM.md`](PIN_DIAGRAM.md).

## Recommended physical arrangement

```text
                FRONT OF ROBOT
        ┌──────────────────────────┐
        │  OLED face               │
        │  XIAO directly behind it │─── RIGHT SIDE: MAX98357A + speaker
        │  servo under head        │
        └──────────────┬───────────┘
                       │ short signal wires
        LEFT N90 motor │  REAR: DRV8833 + power buses  │ RIGHT N90 motor
                       │
        USB-C enters from the rear or side of the XIAO.
```

Keep the OLED and XIAO close together. Keep the DRV8833 close to both motors. Keep the amplifier close to the speaker. Route the three types of wires separately where possible:

- **Signal wires:** OLED, servo, DRV8833 inputs, and I2S audio.
- **5 V wires:** servo and MAX98357A supply.
- **High-current wires:** motor VM and motor outputs.

No loose camera or microphone wires are needed. They are internal to the XIAO Sense expansion board.

## Simplified power architecture

Use only two external power rails in this assembly:

1. **One regulated 5 V rail** for the servo and MAX98357A. A supply rated for at least 2 A is recommended for the first test so servo movement and audio peaks do not collapse the rail.
2. **One separate motor VM rail** for the DRV8833 `VM` and the two N90 motors. Choose the voltage and current for your actual motors and carrier board.

Power the XIAO from USB-C while developing. Connect the USB-powered XIAO ground to the common ground bus. Do not power the XIAO from the servo rail unless the supply path is designed for the XIAO input requirements.

A small screw-terminal block or breadboard power rail makes this simpler:

```text
5V regulated supply +  ──┬── servo red wire
                         └── MAX98357A VIN

5V regulated supply −  ──┬── servo ground
                         ├── MAX98357A GND
                         ├── XIAO GND
                         └── DRV8833 GND and motor-supply negative

Motor supply +         ───── DRV8833 VM
Motor supply −         ───── common ground bus
```

The **motor supply positive** must not join the 5 V rail unless your motor supply is intentionally designed to be 5 V and can handle the motor current. The motor driver’s `VM` voltage must remain within the carrier and motor limits.

## Point-to-point signal harness

Make these connections directly. No level shifter, resistor network, separate enable board, or extra controller is required for this bring-up.

### Harness A — OLED to XIAO

| Wire | From | To |
|---|---|---|
| Red | OLED `VCC` | XIAO `3V3` |
| Black | OLED `GND` | XIAO `GND` |
| Yellow | OLED `SDA` | XIAO `GPIO5 / D4 / SDA` |
| White | OLED `SCL` | XIAO `GPIO6 / D5 / SCL` |

### Harness B — pan servo

| Wire | From | To |
|---|---|---|
| Green/orange | Servo signal | XIAO `GPIO1 / D0` |
| Red | Servo positive | 5 V power bus positive |
| Brown/black | Servo ground | Common ground bus |

Do not connect the servo red wire to XIAO 3V3.

### Harness C — DRV8833 to XIAO and motors

| Wire | From | To |
|---|---|---|
| Blue 1 | DRV8833 `AIN1` | XIAO `GPIO43 / D6 / TX` |
| Blue 2 | DRV8833 `AIN2` | XIAO `GPIO44 / D7 / RX` |
| Blue 3 | DRV8833 `BIN1` | XIAO `GPIO7 / D8` |
| Blue 4 | DRV8833 `BIN2` | XIAO `GPIO8 / D9` |
| Red high-current | Motor supply positive | DRV8833 `VM` |
| Black high-current | Motor supply negative | Common ground bus and DRV8833 `GND` |
| Thin red | XIAO `3V3` | DRV8833 `nSLEEP / SLP` |
| Two motor wires | Left N90 | DRV8833 `AOUT1` and `AOUT2` |
| Two motor wires | Right N90 | DRV8833 `BOUT1` and `BOUT2` |

If your carrier has a separate pin labelled `VCC` or `VLOGIC`, follow that carrier’s silkscreen or datasheet. Some DRV8833 breakout boards expose a logic-supply pin and others do not. Do not guess or bridge it to VM.

### Harness D — MAX98357A to XIAO and speaker

| Wire | From | To |
|---|---|---|
| Yellow | MAX98357A `BCLK` | XIAO `GPIO3 / D2` |
| White | MAX98357A `LRC/LRCK/WS` | XIAO `GPIO4 / D3` |
| Green | MAX98357A `DIN/DATA` | XIAO `GPIO2 / D1` |
| Red | MAX98357A `VIN` | 5 V power bus positive |
| Black | MAX98357A `GND` | Common ground bus |
| Speaker wire 1 | MAX98357A `SPK+` | Speaker positive |
| Speaker wire 2 | MAX98357A `SPK−` | Speaker negative |

Leave `SD/MODE` and `GAIN` as the MAX98357A breakout documentation specifies. Do not connect them to XIAO GPIOs for this stage. If `MCLK` is exposed, leave it disconnected.

> **Speaker rule:** `SPK+` and `SPK−` are bridge-tied outputs. Connect the speaker only between those two terminals. Do not connect either terminal to GND, the XIAO, the motor driver, or another amplifier.

## Minimal assembly checklist

1. Mount the OLED at the front and the XIAO directly behind it.
2. Mount the servo under the head. Keep the servo signal wire short.
3. Mount the DRV8833 at the rear between the left and right motors.
4. Mount the MAX98357A on the right side close to the speaker.
5. Create one 5 V rail and one common ground rail at the rear.
6. Connect the servo and MAX98357A to the 5 V rail.
7. Connect XIAO GND, servo ground, amplifier ground, DRV8833 ground, and both supply negatives to the common ground rail.
8. Connect the motor supply positive only to DRV8833 `VM`.
9. Connect the four DRV8833 input wires and tie `nSLEEP/SLP` to XIAO 3V3.
10. Connect the speaker only between `SPK+` and `SPK−`.
11. Leave the camera and microphone wiring untouched because they are internal to the Sense board.
12. Inspect every red, black, and speaker wire before applying power.

## First power-up order

Use a staged test so a wiring error cannot involve every subsystem at once:

1. Connect only the OLED and XIAO over USB. Run `test oled`.
2. Add the servo 5 V rail and common ground. Run `test servo` and `pan 90`.
3. Add the DRV8833 logic wires and motor supply with the robot lifted. Run `test left` and `test right`.
4. Add the MAX98357A supply and speaker, but do not start playback until the firmware audio test is available.
5. Run `test mic` and `test camera` after the basic hardware remains stable.

Stop immediately if the XIAO resets when the servo or motor starts. That indicates a power or grounding problem, not a software problem.

## References

[1]: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/ "Seeed Studio XIAO ESP32-S3 Getting Started and Pin Map"
[2]: https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts "Adafruit MAX98357 I2S Class-D Mono Amplifier Pinouts"
[3]: https://www.ti.com/lit/gpn/drv8833 "Texas Instruments DRV8833 Dual H-Bridge Motor Driver Datasheet"
