# Local Compile

From the Deskbot repository root:

```powershell
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 firmware/xiao_esp32s3_sense
```

Install the required libraries once:

```powershell
arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306" ESP32Servo
```

For Arduino IDE, open the sketch at `firmware/xiao_esp32s3_sense/xiao_esp32s3_sense.ino` and use the settings in `boards/xiao-esp32s3-sense/README.md`.
