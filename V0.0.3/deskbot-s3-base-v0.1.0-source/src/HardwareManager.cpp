#include "HardwareManager.h"

#include <esp_camera.h>
#include <esp_err.h>

#include "DeskBotConfig.h"

void HardwareManager::begin() {
  pinMode(DeskBotConfig::MOTOR_LEFT_FORWARD_PIN, OUTPUT);
  pinMode(DeskBotConfig::MOTOR_LEFT_REVERSE_PIN, OUTPUT);
  pinMode(DeskBotConfig::MOTOR_RIGHT_FORWARD_PIN, OUTPUT);
  pinMode(DeskBotConfig::MOTOR_RIGHT_REVERSE_PIN, OUTPUT);
  pinMode(DeskBotConfig::LIDAR_XSHUT_PIN, OUTPUT);
  digitalWrite(DeskBotConfig::LIDAR_XSHUT_PIN, HIGH);
  stopMotors();

  Wire.begin(DeskBotConfig::I2C_SDA_PIN, DeskBotConfig::I2C_SCL_PIN);
  Wire.setClock(400000);

  cameraReady_ = initialiseCamera();
  Serial.printf("[hardware] I2C started on SDA=%d / SCL=%d. %s\n",
                DeskBotConfig::I2C_SDA_PIN,
                DeskBotConfig::I2C_SCL_PIN,
                cameraStatus_.c_str());
}

void HardwareManager::loop() {
  if (motorsArmed_ && millis() - lastMotorCommandMs_ > DeskBotConfig::MOTOR_COMMAND_TIMEOUT_MS) {
    stopMotors();
  }
}

void HardwareManager::setMotorArmed(bool armed) {
  motorsArmed_ = armed;
  if (!motorsArmed_) {
    stopMotors();
  } else {
    lastMotorCommandMs_ = millis();
  }
}

bool HardwareManager::motorArmed() const {
  return motorsArmed_;
}

void HardwareManager::setMotors(int leftPwm, int rightPwm) {
  if (!motorsArmed_) {
    stopMotors();
    return;
  }

  leftPwm = constrain(leftPwm, -255, 255);
  rightPwm = constrain(rightPwm, -255, 255);
  writeMotor(DeskBotConfig::MOTOR_LEFT_FORWARD_PIN, DeskBotConfig::MOTOR_LEFT_REVERSE_PIN, leftPwm);
  writeMotor(DeskBotConfig::MOTOR_RIGHT_FORWARD_PIN, DeskBotConfig::MOTOR_RIGHT_REVERSE_PIN, rightPwm);
  lastMotorCommandMs_ = millis();
}

void HardwareManager::stopMotors() {
  analogWrite(DeskBotConfig::MOTOR_LEFT_FORWARD_PIN, 0);
  analogWrite(DeskBotConfig::MOTOR_LEFT_REVERSE_PIN, 0);
  analogWrite(DeskBotConfig::MOTOR_RIGHT_FORWARD_PIN, 0);
  analogWrite(DeskBotConfig::MOTOR_RIGHT_REVERSE_PIN, 0);
}

String HardwareManager::scanI2cJson() {
  String json = "{\"sda\":" + String(DeskBotConfig::I2C_SDA_PIN) + ",\"scl\":" + String(DeskBotConfig::I2C_SCL_PIN) + ",\"devices\":[";
  bool first = true;

  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();
    if (error == 0) {
      if (!first) json += ',';
      json += "\"0x";
      if (address < 16) json += '0';
      json += String(address, HEX);
      json += "\"";
      first = false;
    }
  }
  json += "]}";
  return json;
}

bool HardwareManager::cameraReady() const {
  return cameraReady_;
}

String HardwareManager::cameraStatus() const {
  return cameraStatus_;
}

void HardwareManager::sendCameraSnapshot(WebServer &server) {
  if (!cameraReady_) {
    server.send(503, "application/json", "{\"error\":\"XIAO ESP32-S3 Sense camera not detected\"}");
    return;
  }

  camera_fb_t *frame = esp_camera_fb_get();
  if (!frame) {
    server.send(500, "application/json", "{\"error\":\"Camera capture failed\"}");
    return;
  }

  server.setContentLength(frame->len);
  server.sendHeader("Content-Disposition", "inline; filename=deskbot-s3.jpg");
  server.send(200, "image/jpeg", "");
  WiFiClient client = server.client();
  client.write(frame->buf, frame->len);
  esp_camera_fb_return(frame);
}

bool HardwareManager::initialiseCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = DeskBotConfig::CAM_Y2;
  config.pin_d1 = DeskBotConfig::CAM_Y3;
  config.pin_d2 = DeskBotConfig::CAM_Y4;
  config.pin_d3 = DeskBotConfig::CAM_Y5;
  config.pin_d4 = DeskBotConfig::CAM_Y6;
  config.pin_d5 = DeskBotConfig::CAM_Y7;
  config.pin_d6 = DeskBotConfig::CAM_Y8;
  config.pin_d7 = DeskBotConfig::CAM_Y9;
  config.pin_xclk = DeskBotConfig::CAM_XCLK;
  config.pin_pclk = DeskBotConfig::CAM_PCLK;
  config.pin_vsync = DeskBotConfig::CAM_VSYNC;
  config.pin_href = DeskBotConfig::CAM_HREF;
  config.pin_sccb_sda = DeskBotConfig::CAM_SIOD;
  config.pin_sccb_scl = DeskBotConfig::CAM_SIOC;
  config.pin_pwdn = DeskBotConfig::CAM_PWDN;
  config.pin_reset = DeskBotConfig::CAM_RESET;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = psramFound() ? 2 : 1;
  config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  const esp_err_t result = esp_camera_init(&config);
  if (result != ESP_OK) {
    cameraStatus_ = "Camera optional / unavailable (" + String(esp_err_to_name(result)) + ")";
    return false;
  }

  cameraStatus_ = "XIAO ESP32-S3 Sense camera ready";
  return true;
}

void HardwareManager::writeMotor(int forwardPin, int reversePin, int pwm) {
  if (pwm >= 0) {
    analogWrite(forwardPin, pwm);
    analogWrite(reversePin, 0);
  } else {
    analogWrite(forwardPin, 0);
    analogWrite(reversePin, -pwm);
  }
}
