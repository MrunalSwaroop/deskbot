#pragma once
#include "isabella_state_bitmaps.h"

// Isabella upper-body OLED character module.
// The OLED cannot reproduce full color, so this module prioritizes her
// strongest cues: long dark-hair silhouette, bright bang streak, large eyes,
// blinking, head tilt, shoulder movement, and mouth/gesture changes.

#ifndef ISABELLA_SCREEN_WIDTH
#define ISABELLA_SCREEN_WIDTH 128
#endif
#ifndef ISABELLA_SCREEN_HEIGHT
#define ISABELLA_SCREEN_HEIGHT 64
#endif

extern Adafruit_SSD1306 display;

enum IsabellaState {
  ISABELLA_NEUTRAL,
  ISABELLA_SHY,
  ISABELLA_HAPPY,
  ISABELLA_CONCERNED,
  ISABELLA_LISTENING,
  ISABELLA_SPEAKING,
  ISABELLA_SLEEPING,
  // Derived states reuse the closest reference bitmap and add a small,
  // animated overlay so each Deskbot face command remains visually distinct.
  ISABELLA_CURIOUS,
  ISABELLA_THINKING,
  ISABELLA_SAD,
  ISABELLA_LAUGHING,
  ISABELLA_SURPRISED
};

IsabellaState isabellaState = ISABELLA_NEUTRAL;
uint8_t isabellaFrame = 0;
bool isabellaAuto = true;

const char* isabellaStateName() {
  switch (isabellaState) {
    case ISABELLA_NEUTRAL: return "neutral";
    case ISABELLA_SHY: return "shy";
    case ISABELLA_HAPPY: return "happy";
    case ISABELLA_CONCERNED: return "concerned";
    case ISABELLA_LISTENING: return "listening";
    case ISABELLA_SPEAKING: return "speaking";
    case ISABELLA_SLEEPING: return "sleeping";
    case ISABELLA_CURIOUS: return "curious";
    case ISABELLA_THINKING: return "thinking";
    case ISABELLA_SAD: return "sad";
    case ISABELLA_LAUGHING: return "laughing";
    case ISABELLA_SURPRISED: return "surprised";
  }
  return "neutral";
}

void isabellaLine(int x1, int y1, int x2, int y2, uint8_t width = 1) {
  display.drawLine(x1, y1, x2, y2, WHITE);
  if (width >= 2) display.drawLine(x1, y1 + 1, x2, y2 + 1, WHITE);
}

void drawIsabellaHair(int cx, int top, int bottom, int tilt) {
  // Isabella's hair is black/deep purple in the reel. On a monochrome OLED
  // it must remain mostly black, with only a thin contour and highlights.
  // Long side locks frame a narrower light face instead of filling the screen.
  int c = cx + tilt;
  // No box or helmet outline: the reel shows loose dark hair against a dark
  // background. Use sparse contour strands so the face remains the focus.
  display.drawLine(c - 25, top + 8, c - 27, bottom - 2, WHITE);
  display.drawLine(c + 25, top + 8, c + 27, bottom - 2, WHITE);
  display.drawLine(c - 19, top + 7, c - 21, top + 25, WHITE);
  display.drawLine(c + 19, top + 7, c + 21, top + 25, WHITE);

  // Pale vertical streak in the bangs, on the viewer's left.
  display.drawLine(c - 4, top + 2, c - 4, top + 18, WHITE);
  display.drawLine(c - 3, top + 2, c - 2, top + 16, WHITE);
}

void drawIsabellaShoulders(int cx, int tilt) {
  int c = cx + tilt;
  display.drawFastHLine(c - 25, 58, 50, WHITE);
  display.drawLine(c - 25, 58, c - 10, 49, WHITE);
  display.drawLine(c + 25, 58, c + 10, 49, WHITE);
  // Pale top and slim blue choker become a simple monochrome neck cue.
  display.drawFastHLine(c - 7, 50, 14, WHITE);
  display.drawFastHLine(c - 6, 52, 12, WHITE);
}

void drawIsabellaEyes(int c, int faceY, bool blink, bool wide, int look) {
  if (blink) {
    display.drawLine(c - 14, faceY + 2, c - 5, faceY + 2, BLACK);
    display.drawLine(c + 5, faceY + 2, c + 14, faceY + 2, BLACK);
    return;
  }

  int radius = wide ? 6 : 5;
  display.fillCircle(c - 10, faceY + 2, radius, BLACK);
  display.fillCircle(c + 10, faceY + 2, radius, BLACK);
  display.drawPixel(c - 10 + look, faceY + 2, WHITE);
  display.drawPixel(c + 10 + look, faceY + 2, WHITE);
  display.drawPixel(c - 8 + look, faceY + 1, WHITE);
  display.drawPixel(c + 12 + look, faceY + 1, WHITE);
}

void drawIsabellaMouth(int c, int faceY) {
  if (isabellaState == ISABELLA_HAPPY) {
    display.drawLine(c - 12, faceY + 12, c - 6, faceY + 15, BLACK);
    display.drawLine(c - 6, faceY + 15, c + 6, faceY + 15, BLACK);
    display.drawLine(c + 6, faceY + 15, c + 12, faceY + 12, BLACK);
  } else if (isabellaState == ISABELLA_CONCERNED) {
    display.drawLine(c - 7, faceY + 16, c, faceY + 13, BLACK);
    display.drawLine(c, faceY + 13, c + 7, faceY + 16, BLACK);
  } else if (isabellaState == ISABELLA_SPEAKING) {
    display.fillRoundRect(c - 5, faceY + 11, 10, ((isabellaFrame / 3) & 1) ? 4 : 2, 1, BLACK);
  } else if (isabellaState == ISABELLA_SHY) {
    display.drawFastHLine(c - 4, faceY + 14, 8, BLACK);
  } else {
    display.drawFastHLine(c - 5, faceY + 14, 10, BLACK);
  }
}

void drawIsabellaHandGesture(int c, int faceY, bool handUp, bool nearFace) {
  if (!handUp) return;
  int x = nearFace ? c + 24 : c - 27;
  int y = nearFace ? faceY + 10 : 47;
  int dx = nearFace ? -5 : 5;
  if (nearFace) {
    display.drawLine(x, y, x + dx, y - 14, BLACK);
    display.drawLine(x, y + 1, x + dx, y - 13, BLACK);
    display.drawCircle(x + dx, y - 15, 2, BLACK);
    display.drawPixel(x - 8, y - 18, BLACK);
  } else {
    isabellaLine(x, y, x + dx, y - 14, 2);
    display.drawCircle(x + dx, y - 15, 2, WHITE);
    display.drawPixel(x + 8, y - 18, WHITE);
  }
}

void drawIsabella() {
  // Every state is a direct 128x64 monochrome conversion of a real Isabella
  // reel frame, not a generic avatar reconstruction.
  uint8_t bitmapIndex = 0;
  switch (isabellaState) {
    case ISABELLA_SHY: bitmapIndex = 1; break;
    case ISABELLA_HAPPY:
    case ISABELLA_LAUGHING:
    case ISABELLA_SURPRISED: bitmapIndex = 2; break;
    case ISABELLA_CONCERNED:
    case ISABELLA_THINKING:
    case ISABELLA_SAD: bitmapIndex = 3; break;
    case ISABELLA_LISTENING:
    case ISABELLA_CURIOUS: bitmapIndex = 4; break;
    case ISABELLA_SPEAKING: bitmapIndex = 5; break;
    case ISABELLA_SLEEPING: bitmapIndex = 6; break;
    default: bitmapIndex = 0; break;
  }
  display.drawBitmap(0, 0, ISABELLA_STATE_BITMAPS[bitmapIndex], 128, 64, WHITE);

  // Keep the character reference intact while adding only readable state cues.
  if (isabellaState == ISABELLA_SLEEPING) {
    display.setTextSize(1);
    display.setCursor(103, 2);
    display.print(F("z"));
    display.setCursor(113, 0);
    display.print(F("z"));
  } else if (isabellaState == ISABELLA_SPEAKING) {
    // Talking is never static: alternate a small open/closed mouth and a
    // subtle hand/shoulder cue on successive face frames.
    display.fillRoundRect(56, 43, 16, ((isabellaFrame / 3) & 1) ? 6 : 3, 2, BLACK);
    if ((isabellaFrame / 5) & 1) {
      display.drawLine(98, 46, 105, 39, WHITE);
      display.drawCircle(106, 38, 2, WHITE);
    }
  } else if (isabellaState == ISABELLA_CURIOUS) {
    display.drawLine(49, 28, 60, 25, BLACK);
    display.drawLine(68, 25, 79, 28, BLACK);
    display.fillCircle(64, 44, 3, BLACK);
    display.drawPixel(64 + (((isabellaFrame / 4) & 1) ? 2 : -2), 44, WHITE);
  } else if (isabellaState == ISABELLA_THINKING) {
    display.drawLine(48, 25, 59, 23, BLACK);
    display.drawLine(69, 23, 80, 25, BLACK);
    display.fillCircle(96, 18, 2, WHITE);
    display.fillCircle(103, 13, 3, WHITE);
    display.fillCircle(112, 7, 4, WHITE);
  } else if (isabellaState == ISABELLA_SAD) {
    display.drawLine(49, 24, 59, 28, BLACK);
    display.drawLine(69, 28, 79, 24, BLACK);
    display.drawLine(57, 47, 64, 43, BLACK);
    display.drawLine(64, 43, 71, 47, BLACK);
  } else if (isabellaState == ISABELLA_LAUGHING) {
    display.fillRoundRect(54, 41, 20, 9, 3, BLACK);
    display.drawFastHLine(57, 44, 14, WHITE);
    display.drawLine(97, 49, 104, 40, WHITE);
    display.drawLine(104, 40, 111, 49, WHITE);
  } else if (isabellaState == ISABELLA_SURPRISED) {
    display.drawCircle(64, 45, 6 + ((isabellaFrame / 5) & 1), BLACK);
    display.drawLine(49, 23, 59, 20, BLACK);
    display.drawLine(69, 20, 79, 23, BLACK);
  }
}

void setIsabellaState(IsabellaState next) {
  isabellaState = next;
  isabellaFrame = 0;
}

bool setIsabellaStateByName(String name) {
  name.trim();
  name.toLowerCase();
  if (name == "neutral" || name == "idle") setIsabellaState(ISABELLA_NEUTRAL);
  else if (name == "shy") setIsabellaState(ISABELLA_SHY);
  else if (name == "happy" || name == "joyful") setIsabellaState(ISABELLA_HAPPY);
  else if (name == "concerned" || name == "serious") setIsabellaState(ISABELLA_CONCERNED);
  else if (name == "listening") setIsabellaState(ISABELLA_LISTENING);
  else if (name == "speaking" || name == "talking") setIsabellaState(ISABELLA_SPEAKING);
  else if (name == "sleeping" || name == "sleep") setIsabellaState(ISABELLA_SLEEPING);
  else if (name == "curious") setIsabellaState(ISABELLA_CURIOUS);
  else if (name == "thinking" || name == "working") setIsabellaState(ISABELLA_THINKING);
  else if (name == "sad") setIsabellaState(ISABELLA_SAD);
  else if (name == "laughing" || name == "laugh") setIsabellaState(ISABELLA_LAUGHING);
  else if (name == "surprised") setIsabellaState(ISABELLA_SURPRISED);
  else return false;
  return true;
}

void printIsabellaHelp() {
  Serial.println(F("Isabella states: neutral, shy, happy, concerned, curious, thinking, sad, listening, speaking, laughing, surprised, sleeping"));
  Serial.println(F("Commands: isabella <state>, isabella auto on|off"));
}

void advanceIsabellaPersonality() {
  if (!isabellaAuto) return;
  if (isabellaState == ISABELLA_SLEEPING) return;
  if ((isabellaFrame % 120) != 0 || isabellaFrame == 0) return;
  static uint8_t step = 0;
  const IsabellaState sequence[] = {
    ISABELLA_NEUTRAL,
    ISABELLA_SHY,
    ISABELLA_HAPPY,
    ISABELLA_CONCERNED,
    ISABELLA_LISTENING,
    ISABELLA_SPEAKING
  };
  setIsabellaState(sequence[step]);
  step = (step + 1) % (sizeof(sequence) / sizeof(sequence[0]));
}
