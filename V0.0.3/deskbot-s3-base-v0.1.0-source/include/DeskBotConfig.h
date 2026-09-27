#pragma once

#include <Arduino.h>

namespace DeskBotConfig {

constexpr char FIRMWARE_NAME[] = "DeskBot-S3 Base";
constexpr char FIRMWARE_VERSION[] = "0.1.0";
constexpr char BOARD_NAME[] = "Seeed XIAO ESP32-S3 / Sense";

// Captive portal appears only when saved Wi-Fi cannot be reached.
constexpr char SETUP_AP_SSID[] = "DeskBot-S3-Setup";
constexpr char SETUP_AP_PASSWORD[] = "deskbot-s3";
constexpr char MDNS_HOSTNAME[] = "deskbot-s3";

// XIAO ESP32-S3 expansion-pin defaults.
// These are intentionally isolated here so each hardware module can be remapped
// without changing network or control code. Motors start disabled after every boot.
constexpr int MOTOR_LEFT_FORWARD_PIN = 1;   // XIAO D0
constexpr int MOTOR_LEFT_REVERSE_PIN = 2;   // XIAO D1
constexpr int MOTOR_RIGHT_FORWARD_PIN = 3;  // XIAO D2
constexpr int MOTOR_RIGHT_REVERSE_PIN = 4;  // XIAO D3
constexpr int I2C_SDA_PIN = 5;              // XIAO D4
constexpr int I2C_SCL_PIN = 6;              // XIAO D5
constexpr int LIDAR_XSHUT_PIN = 7;          // XIAO D8, optional VL53L5CX enable
constexpr int MOTOR_COMMAND_TIMEOUT_MS = 500;

// XIAO ESP32-S3 Sense OV2640 camera pins. Camera initialisation is optional:
// plain XIAO ESP32-S3 boards continue normally when no camera is attached.
constexpr int CAM_PWDN = -1;
constexpr int CAM_RESET = -1;
constexpr int CAM_XCLK = 10;
constexpr int CAM_SIOD = 40;
constexpr int CAM_SIOC = 39;
constexpr int CAM_Y9 = 48;
constexpr int CAM_Y8 = 11;
constexpr int CAM_Y7 = 12;
constexpr int CAM_Y6 = 14;
constexpr int CAM_Y5 = 16;
constexpr int CAM_Y4 = 18;
constexpr int CAM_Y3 = 17;
constexpr int CAM_Y2 = 15;
constexpr int CAM_VSYNC = 38;
constexpr int CAM_HREF = 47;
constexpr int CAM_PCLK = 13;

}  // namespace DeskBotConfig
