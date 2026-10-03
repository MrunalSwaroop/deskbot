#pragma once

struct PushToTalkState {
  bool pressed;
  bool listening;
};

inline PushToTalkState pushToTalkState(bool pressed) {
  return {pressed, pressed};
}
