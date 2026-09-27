#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <Wire.h>

class HardwareManager {
 public:
  void begin();
  void loop();
  void setMotorArmed(bool armed);
  bool motorArmed() const;
  void setMotors(int leftPwm, int rightPwm);
  void stopMotors();
  String scanI2cJson();
  bool cameraReady() const;
  String cameraStatus() const;
  void sendCameraSnapshot(WebServer &server);

 private:
  bool initialiseCamera();
  void writeMotor(int forwardPin, int reversePin, int pwm);

  bool motorsArmed_ = false;
  bool cameraReady_ = false;
  uint32_t lastMotorCommandMs_ = 0;
  String cameraStatus_ = "Camera not initialised";
};
