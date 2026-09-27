#pragma once

struct ServoMotionPort {
  void (*setAngle)(int angle);
  int minAngle;
  int maxAngle;
};

inline int clampServoAngle(const ServoMotionPort& port, int angle) {
  if (angle < port.minAngle) return port.minAngle;
  if (angle > port.maxAngle) return port.maxAngle;
  return angle;
}
