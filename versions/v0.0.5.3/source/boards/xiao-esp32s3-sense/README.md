# Seeed XIAO ESP32-S3 Sense Board Profile

This is the active Deskbot hardware target.

## Arduino IDE settings

Use the following settings for the first USB upload:

| Setting | Value |
|---|---|
| Board | `XIAO_ESP32S3` |
| Flash size | `8MB (64Mb)` |
| PSRAM | `OPI PSRAM` |
| Partition | An OTA-capable `Default with spiffs` scheme |
| Upload mode | `UART0 / Hardware CDC` |
| USB mode | `Hardware CDC and JTAG` |
| Port | The detected XIAO USB port |
| Serial monitor | `115200 baud` |

Keep the motor supply switched off and lift the robot during the first hardware tests. Keep the USB recovery path available even after OTA works.

## Board contract

The board profile uses the pin map in `pinmap.h`. Do not edit peripheral GPIO assignments in the firmware integration sketch. When a second board is added, create another profile directory and another firmware target rather than adding conditional pin constants throughout the active firmware.
