#pragma once

struct TrackMotionCommand {
  int leftPwm;
  int rightPwm;
  unsigned long durationMs;
};

inline TrackMotionCommand boundedTrackCommand(int leftPwm, int rightPwm, unsigned long durationMs) {
  TrackMotionCommand command{};
  command.leftPwm = leftPwm < -255 ? -255 : (leftPwm > 255 ? 255 : leftPwm);
  command.rightPwm = rightPwm < -255 ? -255 : (rightPwm > 255 ? 255 : rightPwm);
  command.durationMs = durationMs > 1500UL ? 1500UL : durationMs;
  return command;
}
