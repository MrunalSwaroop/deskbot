#pragma once

struct Max98357aPins {
  int bclk;
  int lrck;
  int data;
};

inline Max98357aPins max98357aPins(int bclk, int lrck, int data) {
  return {bclk, lrck, data};
}
