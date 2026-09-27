# Stage 2: face plus pan/tilt neck

The face-only test is complete. This stage adds one or two micro servos while keeping the OLED face and serial commands working. Motors remain disconnected.

## Firmware

Use:

- `face_neck_test.ino`
- `eyes.h`

Keep both files in the same Arduino sketch folder. Install the **Servo** library if Arduino IDE does not already provide it for the UNO R4.

## Servo wiring

The sketch uses conservative angle limits while the mechanism is untested.

| Servo | Signal | Power | Ground |
|---|---:|---|---|
| Pan servo | UNO D3 | External regulated 5 V recommended | Common GND with UNO |
| Tilt servo | UNO D4 | External regulated 5 V recommended | Common GND with UNO |

A typical hobby servo has three wires: signal, positive supply, and ground. Wire colors vary by manufacturer, so follow the label or datasheet rather than assuming colors.

For a first test, connect only the **pan servo** to D3. Leave D4 disconnected if you have not mounted the tilt servo. The firmware can still run with the second servo signal disconnected.

The servo supply ground must be connected to UNO GND. Do not connect a separate supply's positive output to an UNO GPIO pin. If the OLED resets when the servo moves, stop and use a better regulated supply with a shared ground.

## Mechanical setup

Before attaching the horn or neck linkage:

1. Upload the sketch with the servo arm unloaded.
2. The servo will move to its center position, approximately 90 degrees.
3. Power off the system.
4. Attach the servo horn so the mechanism is physically centered.
5. Keep the first movement range small.

The default software limits are:

```text
Pan: 60 to 120 degrees
Tilt: 65 to 115 degrees
```

These are intentionally not the full servo range. Expand them only after checking that the neck cannot hit its stops.

## Upload and test

1. Keep the OLED wiring unchanged.
2. Put `face_neck_test.ino` and `eyes.h` in the same folder.
3. Select **Arduino UNO R4 WiFi**.
4. Select the UNO USB port.
5. Upload.
6. Open Serial Monitor at **115200 baud**.
7. Use **Newline** or **Both NL & CR** as the line ending.

The OLED should still boot with the Rocky greeting and idle face.

Test the pan servo using:

```text
pan 90
pan 75
pan 105
look left
look center
look right
```

Test the tilt servo using:

```text
tilt 90
tilt 75
tilt 105
```

If the servo moves in the opposite direction from the expected physical direction, do not change the limits first. Reverse the servo horn position or invert the mapping in the firmware after the mechanism is understood.

## Face and neck combinations

The following commands intentionally combine OLED and neck behavior:

```text
happy
look left
listen
look center
speak
look right
sleep
neck center
```

The expected behavior is that the face communicates attention while the neck makes a small, safe movement. The speaking mouth continues its animation independently.

## What success looks like

This stage is complete when:

- The OLED still displays the face correctly.
- The pan servo reaches left, center, and right without buzzing at a mechanical stop.
- The tilt servo reaches center and small up/down angles without binding.
- Serial commands remain responsive while the face blinks or the mouth animates.
- A power reset returns both servos to center.

Do not add the L298N or motor battery until this stage passes. The next stage will be a motor-driver test with the tracks lifted off the table.
