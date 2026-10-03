#pragma once

#include <cstring>

struct FleetRolloutPolicy {
  const char* rollout;
  const char* selectedBoardIds;
};

inline bool fleetRolloutIsAllowed(const char* rollout, const char* selectedBoardIds, const char* boardId) {
  if (!rollout || !selectedBoardIds || !boardId) return false;
  if (rollout[0] == 'a') return true;
  return strstr(selectedBoardIds, boardId) != nullptr;
}
