#pragma once

// Rocky XIAO Desk Buddy hardware configuration.
// Change pins here when moving the electronics to another enclosure.
// Keep all GPIO numbers as ESP32 GPIO numbers, not Arduino digital labels.

#define ROCKY_OLED_SDA 5
#define ROCKY_OLED_SCL 6
#define ROCKY_OLED_ADDRESS 0x3C

// Pan servo signal. Power the servo from a separate regulated 5 V supply.
#define ROCKY_SERVO_PIN 1

// DRV8833: PWM is applied directly to one input of each H-bridge;
// the opposite input is held LOW. Tie the driver's nSLEEP/SLP to 3V3.
// External signal pins are grouped on adjacent XIAO headers D6-D9.
// Leave nFAULT unconnected for this bring-up, or connect it later to a spare input.
#define ROCKY_LEFT_IN1 43  // D6 / TX -> AIN1
#define ROCKY_LEFT_IN2 44  // D7 / RX -> AIN2
#define ROCKY_RIGHT_IN1 7  // D8 -> BIN1
#define ROCKY_RIGHT_IN2 8  // D9 -> BIN2

// Sense peripherals and adjacent audio block: do not reuse these pins.
#define ROCKY_PDM_MIC_CLOCK 42
#define ROCKY_PDM_MIC_DATA 41
#define ROCKY_AUDIO_BCLK 3  // D2
#define ROCKY_AUDIO_LRCK 4
#define ROCKY_AUDIO_DATA 2

// Sense camera and SD-card pins remain reserved. This bring-up does not use SD.
#define ROCKY_SD_CS 21

// Default motion and timing values.
#define ROCKY_DEFAULT_MOTOR_SPEED 170
#define ROCKY_MOTOR_COMMAND_TIMEOUT_MS 1500
#define ROCKY_FACE_FRAME_MS 90
#define ROCKY_WIFI_RETRY_MS 15000
#define ROCKY_ISABELLA_MOOD_MIN_MS 2500
#define ROCKY_ISABELLA_MOOD_MAX_MS 6500

// Set to true only after the mechanism is physically checked.
#define ROCKY_ALLOW_FULL_SERVO_RANGE true
#define ROCKY_SERVO_MIN_ANGLE 0
#define ROCKY_SERVO_MAX_ANGLE 180

// Set to false if your motor module's left/right orientation is reversed.
#define ROCKY_LEFT_MOTOR_INVERT false
// Mirrored track assemblies usually need the right electrical direction inverted
// so equal commands produce the same physical forward direction.
#define ROCKY_RIGHT_MOTOR_INVERT true

// The firmware uses an active-high DRV8833 nSLEEP connection externally.
// If you wire nSLEEP to a GPIO later, add that pin and initialization here.
#define ROCKY_DRV8833_SLEEP_TIED_HIGH true

// Wi-Fi credentials are intentionally not stored in this header.
// Configure them through the device setup AP and local dashboard.
#define ROCKY_DEFAULT_TIMEZONE_OFFSET_SECONDS 19800

// XIAO ESP32-S3 Sense camera. These GPIOs are occupied by the camera connector.
#define ROCKY_CAMERA_ENABLED true
#define ROCKY_CAMERA_PWDN -1
#define ROCKY_CAMERA_RESET -1
#define ROCKY_CAMERA_XCLK 10
#define ROCKY_CAMERA_SIOD 40
#define ROCKY_CAMERA_SIOC 39
#define ROCKY_CAMERA_Y9 48
#define ROCKY_CAMERA_Y8 11
#define ROCKY_CAMERA_Y7 12
#define ROCKY_CAMERA_Y6 14
#define ROCKY_CAMERA_Y5 16
#define ROCKY_CAMERA_Y4 18
#define ROCKY_CAMERA_Y3 17
#define ROCKY_CAMERA_Y2 15
#define ROCKY_CAMERA_VSYNC 38
#define ROCKY_CAMERA_HREF 47
#define ROCKY_CAMERA_PCLK 13
#define ROCKY_CAMERA_FRAME_SIZE FRAMESIZE_QVGA
#define ROCKY_CAMERA_JPEG_QUALITY 12

// Pin reservations summary:
// Adjacent external header map: servo D0; audio D1/D2/D3; OLED D4/D5;
// DRV8833 D6/D7/D8/D9. GPIO9/D10 remains spare but the Sense SD bus is not used.
// PDM 42/41; SD-CS 21 reserved and unused.
// Camera pins 10/11/12/13/14/15/16/17/18/38/39/40/47/48 are occupied by the Sense camera.

#if ROCKY_LEFT_IN1 == ROCKY_OLED_SDA || ROCKY_LEFT_IN1 == ROCKY_OLED_SCL || ROCKY_LEFT_IN2 == ROCKY_OLED_SDA || ROCKY_LEFT_IN2 == ROCKY_OLED_SCL || ROCKY_RIGHT_IN1 == ROCKY_OLED_SDA || ROCKY_RIGHT_IN1 == ROCKY_OLED_SCL || ROCKY_RIGHT_IN2 == ROCKY_OLED_SDA || ROCKY_RIGHT_IN2 == ROCKY_OLED_SCL
#error "DRV8833 pin overlaps OLED I2C pins"
#endif

#if ROCKY_LEFT_IN1 == ROCKY_PDM_MIC_CLOCK || ROCKY_LEFT_IN1 == ROCKY_PDM_MIC_DATA || ROCKY_LEFT_IN2 == ROCKY_PDM_MIC_CLOCK || ROCKY_LEFT_IN2 == ROCKY_PDM_MIC_DATA || ROCKY_RIGHT_IN1 == ROCKY_PDM_MIC_CLOCK || ROCKY_RIGHT_IN1 == ROCKY_PDM_MIC_DATA || ROCKY_RIGHT_IN2 == ROCKY_PDM_MIC_CLOCK || ROCKY_RIGHT_IN2 == ROCKY_PDM_MIC_DATA
#error "DRV8833 pin overlaps future Sense PDM microphone pins"
#endif

#if ROCKY_LEFT_IN1 == ROCKY_AUDIO_BCLK || ROCKY_LEFT_IN1 == ROCKY_AUDIO_LRCK || ROCKY_LEFT_IN1 == ROCKY_AUDIO_DATA || ROCKY_LEFT_IN2 == ROCKY_AUDIO_BCLK || ROCKY_LEFT_IN2 == ROCKY_AUDIO_LRCK || ROCKY_LEFT_IN2 == ROCKY_AUDIO_DATA || ROCKY_RIGHT_IN1 == ROCKY_AUDIO_BCLK || ROCKY_RIGHT_IN1 == ROCKY_AUDIO_LRCK || ROCKY_RIGHT_IN1 == ROCKY_AUDIO_DATA || ROCKY_RIGHT_IN2 == ROCKY_AUDIO_BCLK || ROCKY_RIGHT_IN2 == ROCKY_AUDIO_LRCK || ROCKY_RIGHT_IN2 == ROCKY_AUDIO_DATA
#error "DRV8833 pin overlaps future audio pins"
#endif

#if ROCKY_LEFT_IN1 == ROCKY_SERVO_PIN || ROCKY_LEFT_IN2 == ROCKY_SERVO_PIN || ROCKY_RIGHT_IN1 == ROCKY_SERVO_PIN || ROCKY_RIGHT_IN2 == ROCKY_SERVO_PIN
#error "DRV8833 pin overlaps servo pin"
#endif

#if ROCKY_LEFT_IN1 == ROCKY_RIGHT_IN1 || ROCKY_LEFT_IN1 == ROCKY_RIGHT_IN2 || ROCKY_LEFT_IN2 == ROCKY_RIGHT_IN1 || ROCKY_LEFT_IN2 == ROCKY_RIGHT_IN2 || ROCKY_RIGHT_IN1 == ROCKY_RIGHT_IN2 || ROCKY_LEFT_IN1 == ROCKY_LEFT_IN2
#error "DRV8833 pins must be unique"
#endif

#if !ROCKY_DRV8833_SLEEP_TIED_HIGH
#error "This bring-up requires DRV8833 nSLEEP/SLP tied HIGH to 3V3"
#endif

#if !ROCKY_ALLOW_FULL_SERVO_RANGE
#error "Set an explicit servo range policy before compiling"
#endif

#if ROCKY_SERVO_MIN_ANGLE < 0 || ROCKY_SERVO_MAX_ANGLE > 180 || ROCKY_SERVO_MIN_ANGLE >= ROCKY_SERVO_MAX_ANGLE
#error "Servo angle limits are invalid"
#endif

#if ROCKY_DEFAULT_MOTOR_SPEED < 0 || ROCKY_DEFAULT_MOTOR_SPEED > 255
#error "Default motor speed must be 0..255"
#endif

#if ROCKY_MOTOR_COMMAND_TIMEOUT_MS < 250
#error "Motor timeout is too short for safe bring-up"
#endif

#if ROCKY_ISABELLA_MOOD_MIN_MS < 500 || ROCKY_ISABELLA_MOOD_MIN_MS > ROCKY_ISABELLA_MOOD_MAX_MS
#error "Isabella mood timing is invalid"
#endif

#if ROCKY_SD_CS == ROCKY_LEFT_IN1 || ROCKY_SD_CS == ROCKY_LEFT_IN2 || ROCKY_SD_CS == ROCKY_RIGHT_IN1 || ROCKY_SD_CS == ROCKY_RIGHT_IN2
#error "Do not use the Sense SD-CS pin for the DRV8833 bring-up"
#endif

#if ROCKY_SD_CS == ROCKY_SERVO_PIN || ROCKY_SD_CS == ROCKY_OLED_SDA || ROCKY_SD_CS == ROCKY_OLED_SCL
#error "Reserved SD-CS pin conflicts with another peripheral"
#endif

#if ROCKY_PDM_MIC_CLOCK == ROCKY_PDM_MIC_DATA || ROCKY_AUDIO_BCLK == ROCKY_AUDIO_LRCK || ROCKY_AUDIO_BCLK == ROCKY_AUDIO_DATA || ROCKY_AUDIO_LRCK == ROCKY_AUDIO_DATA
#error "Future audio pins must be unique"
#endif

#if ROCKY_OLED_SDA == ROCKY_OLED_SCL
#error "OLED I2C pins must be unique"
#endif

#if ROCKY_OLED_ADDRESS != 0x3C && ROCKY_OLED_ADDRESS != 0x3D
#error "OLED address must be 0x3C or 0x3D"
#endif

#if ROCKY_SERVO_PIN == ROCKY_OLED_SDA || ROCKY_SERVO_PIN == ROCKY_OLED_SCL
#error "Servo pin overlaps OLED I2C"
#endif

#if ROCKY_SERVO_PIN == ROCKY_PDM_MIC_CLOCK || ROCKY_SERVO_PIN == ROCKY_PDM_MIC_DATA || ROCKY_SERVO_PIN == ROCKY_AUDIO_BCLK || ROCKY_SERVO_PIN == ROCKY_AUDIO_LRCK || ROCKY_SERVO_PIN == ROCKY_AUDIO_DATA
#error "Servo pin overlaps a reserved future peripheral"
#endif

#if ROCKY_OLED_SDA == ROCKY_PDM_MIC_CLOCK || ROCKY_OLED_SDA == ROCKY_PDM_MIC_DATA || ROCKY_OLED_SCL == ROCKY_PDM_MIC_CLOCK || ROCKY_OLED_SCL == ROCKY_PDM_MIC_DATA
#error "OLED pins overlap future PDM microphone pins"
#endif

#if ROCKY_OLED_SDA == ROCKY_AUDIO_BCLK || ROCKY_OLED_SDA == ROCKY_AUDIO_LRCK || ROCKY_OLED_SDA == ROCKY_AUDIO_DATA || ROCKY_OLED_SCL == ROCKY_AUDIO_BCLK || ROCKY_OLED_SCL == ROCKY_AUDIO_LRCK || ROCKY_OLED_SCL == ROCKY_AUDIO_DATA
#error "OLED pins overlap future audio pins"
#endif

#if ROCKY_SERVO_PIN == ROCKY_SD_CS
#error "Servo pin overlaps reserved SD-CS"
#endif

#if ROCKY_OLED_SDA == ROCKY_SD_CS || ROCKY_OLED_SCL == ROCKY_SD_CS
#error "OLED pin overlaps reserved SD-CS"
#endif

#if ROCKY_PDM_MIC_CLOCK == ROCKY_SD_CS || ROCKY_PDM_MIC_DATA == ROCKY_SD_CS || ROCKY_AUDIO_BCLK == ROCKY_SD_CS || ROCKY_AUDIO_LRCK == ROCKY_SD_CS || ROCKY_AUDIO_DATA == ROCKY_SD_CS
#error "Future peripheral pin overlaps reserved SD-CS"
#endif

#if ROCKY_PDM_MIC_CLOCK == ROCKY_AUDIO_BCLK || ROCKY_PDM_MIC_CLOCK == ROCKY_AUDIO_LRCK || ROCKY_PDM_MIC_CLOCK == ROCKY_AUDIO_DATA || ROCKY_PDM_MIC_DATA == ROCKY_AUDIO_BCLK || ROCKY_PDM_MIC_DATA == ROCKY_AUDIO_LRCK || ROCKY_PDM_MIC_DATA == ROCKY_AUDIO_DATA
#error "PDM and audio pins must remain unique"
#endif

#if ROCKY_SERVO_MIN_ANGLE != 0 || ROCKY_SERVO_MAX_ANGLE != 180
// This warning is intentionally compile-time visible if you choose a narrower range.
#warning "Servo range is narrower than the requested 0..180 test range"
#endif

#if ROCKY_LEFT_MOTOR_INVERT != false && ROCKY_LEFT_MOTOR_INVERT != true
#error "Left motor invert must be true or false"
#endif

#if ROCKY_RIGHT_MOTOR_INVERT != false && ROCKY_RIGHT_MOTOR_INVERT != true
#error "Right motor invert must be true or false"
#endif

#if ROCKY_FACE_FRAME_MS < 30
#error "Face frame interval is too short for stable OLED bring-up"
#endif

#if ROCKY_WIFI_RETRY_MS < 1000
#error "Wi-Fi retry interval is too short"
#endif

#if ROCKY_SERVO_PIN < 0 || ROCKY_SERVO_PIN > 48 || ROCKY_LEFT_IN1 < 0 || ROCKY_LEFT_IN1 > 48 || ROCKY_LEFT_IN2 < 0 || ROCKY_LEFT_IN2 > 48 || ROCKY_RIGHT_IN1 < 0 || ROCKY_RIGHT_IN1 > 48 || ROCKY_RIGHT_IN2 < 0 || ROCKY_RIGHT_IN2 > 48
#error "GPIO number outside ESP32-S3 range"
#endif

// This file deliberately contains no Wi-Fi password, API key, or cloud endpoint.
// Credentials belong in NVS via the local setup page.
