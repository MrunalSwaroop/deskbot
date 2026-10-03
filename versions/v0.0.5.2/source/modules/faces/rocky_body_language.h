#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "rocky_bump_frames.h"

#ifndef ROCKY_DEFAULT_FRAME_DELAY
#define ROCKY_DEFAULT_FRAME_DELAY 150
#endif
#ifndef ROCKY_PERSONALITY_DEFAULT
#define ROCKY_PERSONALITY_DEFAULT 1
#endif

namespace RockyBodyLanguage {

// This header contains Rocky's state model, geometry primitives, behavior
// functions, personality scheduler, and Serial command parser. It is included
// once by rocky_modular_base.ino.
extern Adafruit_SSD1306 &display;

enum BodyState {
  IDLE,
  CURIOUS,
  GREETING,
  LISTENING,
  WALK_FORWARD,
  WALK_BACK,
  PACE,
  COME_HERE,
  GO_BACK,
  SLEEPING,
  WAKING_UP,
  DANCE,
  IDLE_ACTIVITY
};

enum IdleActivity {
  ACT_YOYO,
  ACT_FETCH
};

BodyState state = IDLE;
IdleActivity activity = ACT_YOYO;

uint8_t frame = 0;
uint16_t frameDelay = ROCKY_DEFAULT_FRAME_DELAY;
unsigned long lastFrame = 0;
unsigned long lastInteraction = 0;
unsigned long nextPersonalityEvent = 0;
unsigned long returnToIdleAt = 0;
int walkX = 64;
int paceDirection = 1;
bool playing = true;
bool rockyPersonality = ROCKY_PERSONALITY_DEFAULT;
bool personalityOwnedState = false;
uint8_t personalityStep = 0;

// ---------- Low-level Rocky geometry ----------

void thickLine(int x1, int y1, int x2, int y2, uint8_t width = 2) {
  display.drawLine(x1, y1, x2, y2, WHITE);
  if (width >= 2) display.drawLine(x1, y1 + 1, x2, y2 + 1, WHITE);
  if (width >= 3) display.drawLine(x1, y1 - 1, x2, y2 - 1, WHITE);
}

// A short hinge/collar represents a joint. It is not a large circle.
void drawJoint(int x, int y) {
  display.fillRect(x - 3, y - 1, 7, 3, WHITE);
  display.drawPixel(x, y, BLACK);
}

void drawFoot(int x, int y, int direction) {
  thickLine(x, y, x + direction * 5, y - 3, 1);
  thickLine(x + direction * 2, y, x + direction * 7, y + 1, 1);
}

void drawHand(int x, int y, int direction, bool open, bool wave) {
  // Rocky's two-pronged hand shape. Wave changes only the finger position.
  thickLine(x, y, x + direction * 6, y - 8, 2);
  if (open || wave) thickLine(x + direction * 2, y - 1, x + direction * 9, y - 7, 2);
  else thickLine(x + direction * 2, y - 1, x + direction * 7, y - 4, 2);
  if (wave) thickLine(x + direction * 3, y - 2, x + direction * 8, y + 1, 1);
}

void drawLeg(int hipX, int hipY, int kneeX, int kneeY, int footX, int footY, bool lifted) {
  int finalY = footY - (lifted ? 4 : 0);
  thickLine(hipX, hipY, kneeX, kneeY, 3);
  drawJoint(kneeX, kneeY);
  thickLine(kneeX, kneeY, footX, finalY, 3);
  drawFoot(footX, finalY, footX < 64 ? -1 : 1);
}

void drawConnectedTorso(int cx, int top, int width, int height) {
  int left = cx - width / 2;
  int right = cx + width / 2;
  int bottom = top + height;

  // Restore the earlier broad, filled Rocky torso that matched the OLED
  // reference more closely. It stays connected to all limbs in every pose.
  display.fillRoundRect(left, top, width, height - 7, 7, WHITE);
  display.fillTriangle(left + 3, bottom - 9, cx, bottom + 2, right - 3, bottom - 9, WHITE);
  display.drawRoundRect(left, top, width, height - 7, 7, WHITE);
}

void drawGround(int y = 63) {
  display.drawFastHLine(16, y, 96, WHITE);
}

// ---------- Individual body states ----------

void drawIdle(int cx, int top) {
  bool smallBob = ((frame / 5) & 1) != 0;
  int y = top + (smallBob ? 1 : 0);
  drawConnectedTorso(cx, y, 42, 28);
  int shoulderY = y + 7;
  int hipY = y + 22;

  thickLine(cx - 16, shoulderY, cx - 28, 23, 3);
  thickLine(cx + 16, shoulderY, cx + 28, 23, 3);
  drawJoint(cx - 28, 23);
  drawJoint(cx + 28, 23);
  drawHand(cx - 34, 18, -1, true, false);
  drawHand(cx + 34, 18, 1, true, false);

  drawLeg(cx - 13, hipY, cx - 25, 46, cx - 35, 60, false);
  drawLeg(cx - 7, hipY + 1, cx - 13, 49, cx - 18, 61, false);
  drawLeg(cx + 13, hipY, cx + 25, 46, cx + 35, 60, false);
  drawLeg(cx + 7, hipY + 1, cx + 13, 49, cx + 18, 61, false);
  drawGround();
}

void drawCurious(int cx, int top) {
  // New curious pose: Rocky crouches, shifts weight, reaches one hand toward
  // the nearby object, and raises the opposite hand as if examining it.
  int sway = ((frame / 4) & 1) ? 3 : -2;
  int c = cx + sway;
  int y = top + 7 + (((frame / 5) & 1) ? 1 : 0);
  drawConnectedTorso(c, y, 40, 23);
  int shoulderY = y + 7;
  int hipY = y + 19;

  thickLine(c - 15, shoulderY, c - 32, 34, 3);
  drawJoint(c - 32, 34);
  drawHand(c - 38, 29, -1, false, false);

  thickLine(c + 15, shoulderY, c + 29, 18, 3);
  drawJoint(c + 29, 18);
  drawHand(c + 35, 13, 1, true, false);

  // Weight shift: the near front limb is lifted slightly while the other
  // three support the crouched torso.
  drawLeg(c - 13, hipY, c - 28, 47, c - 40, 58, true);
  drawLeg(c - 7, hipY, c - 12, 51, c - 18, 61, false);
  drawLeg(c + 13, hipY, c + 28, 46, c + 40, 60, false);
  drawLeg(c + 7, hipY, c + 12, 50, c + 18, 61, false);
  drawGround();
}

void drawGreeting(int cx, int top) {
  // Exactly one hand is raised high and waves. The other remains down.
  bool wave = ((frame / 3) & 1) != 0;
  int y = top + (((frame / 8) & 1) ? 1 : 0);
  drawConnectedTorso(cx, y, 42, 29);
  int shoulderY = y + 6;
  int hipY = y + 22;

  thickLine(cx - 16, shoulderY, cx - 31, 14, 3);
  drawJoint(cx - 31, 14);
  drawHand(cx - 37, 9, -1, true, wave);

  thickLine(cx + 16, shoulderY, cx + 30, 29, 3);
  drawJoint(cx + 30, 29);
  drawHand(cx + 36, 24, 1, false, false);

  drawLeg(cx - 13, hipY, cx - 27, 47, cx - 37, 60, false);
  drawLeg(cx - 7, hipY, cx - 12, 50, cx - 17, 61, false);
  drawLeg(cx + 13, hipY, cx + 27, 47, cx + 37, 60, false);
  drawLeg(cx + 7, hipY, cx + 12, 50, cx + 17, 61, false);
  drawGround();
}

void drawListening(int cx, int top) {
  // Listening comes toward the viewer: the torso grows larger and lower,
  // while all limbs remain attached to the same enlarged body mass.
  uint8_t approach = frame < 8 ? frame : 8;
  int width = 42 + approach * 2;
  int height = 28 + approach * 2;
  int y = 20 - approach;
  int c = cx;
  drawConnectedTorso(c, y, width, height);

  int shoulderY = y + 8 + approach / 2;
  int hipY = y + height - 5;
  int reach = 28 + approach;

  // Arms open slightly toward the viewer as Rocky listens closely.
  thickLine(c - width / 2 + 5, shoulderY, c - reach, 25 + approach / 2, 3);
  thickLine(c + width / 2 - 5, shoulderY, c + reach, 25 + approach / 2, 3);
  drawJoint(c - reach, 25 + approach / 2);
  drawJoint(c + reach, 25 + approach / 2);
  drawHand(c - reach - 6, 20 + approach / 2, -1, false, false);
  drawHand(c + reach + 6, 20 + approach / 2, 1, false, false);

  // The lower body also enlarges slightly, making the approach readable.
  drawLeg(c - width / 2 + 10, hipY, c - 30 - approach / 2, 47, c - 42 - approach / 2, 60, false);
  drawLeg(c - 8, hipY + 1, c - 15, 50, c - 22, 61, false);
  drawLeg(c + width / 2 - 10, hipY, c + 30 + approach / 2, 47, c + 42 + approach / 2, 60, false);
  drawLeg(c + 8, hipY + 1, c + 15, 50, c + 22, 61, false);
  drawGround();
}

void drawWalking(int cx, int top, bool walkingBack) {
  // cx is actual horizontal travel position. The whole body translates.
  bool step = ((frame / 2) & 1) != 0;
  int y = top + (step ? 1 : 0);
  drawConnectedTorso(cx, y, 43, 28);
  int shoulderY = y + 7;
  int hipY = y + 22;

  int armSwing = step ? 4 : -2;
  thickLine(cx - 16, shoulderY, cx - 29, 22 + armSwing, 3);
  thickLine(cx + 16, shoulderY, cx + 29, 22 - armSwing, 3);
  drawJoint(cx - 29, 22 + armSwing);
  drawJoint(cx + 29, 22 - armSwing);
  drawHand(cx - 35, 16 + armSwing, -1, true, false);
  drawHand(cx + 35, 16 - armSwing, 1, true, false);

  bool leftForward = walkingBack ? !step : step;
  bool rightForward = !leftForward;
  drawLeg(cx - 13, hipY, cx - 29, 45, cx - 39, 55, !leftForward);
  drawLeg(cx - 7, hipY, cx - 11, 50, cx - 18, 61, leftForward);
  drawLeg(cx + 13, hipY, cx + 29, 45, cx + 39, 55, !rightForward);
  drawLeg(cx + 7, hipY, cx + 11, 50, cx + 18, 61, rightForward);
  drawGround();
}

void drawApproach(int cx, bool comingHere) {
  // Come-here grows Rocky toward the viewer; go-back shrinks him away.
  // Both actions scale the complete body together instead of separating the
  // torso from the limbs.
  uint8_t phase = frame < 12 ? frame : 12;
  uint8_t depth = comingHere ? phase : 12 - phase;
  int width = 28 + depth * 2;
  int top = 29 - depth;
  int height = 20 + depth;
  int c = cx;
  drawConnectedTorso(c, top, width, height);

  int shoulderY = top + 6;
  int hipY = top + height - 3;
  int armReach = 19 + depth;
  int handY = 25 - depth / 2;
  thickLine(c - width / 2 + 4, shoulderY, c - armReach, handY, 3);
  thickLine(c + width / 2 - 4, shoulderY, c + armReach, handY, 3);
  drawJoint(c - armReach, handY);
  drawJoint(c + armReach, handY);
  drawHand(c - armReach - 6, handY - 5, -1, true, false);
  drawHand(c + armReach + 6, handY - 5, 1, true, false);

  int legSpread = 22 + depth / 2;
  drawLeg(c - width / 2 + 7, hipY, c - legSpread, 47, c - legSpread - 7, 60, false);
  drawLeg(c - 5, hipY + 1, c - 10, 50, c - 15, 61, false);
  drawLeg(c + width / 2 - 7, hipY, c + legSpread, 47, c + legSpread + 7, 60, false);
  drawLeg(c + 5, hipY + 1, c + 10, 50, c + 15, 61, false);
  drawGround();
}

void drawSleeping(int cx, int top) {
  // Use the actual reference-derived low Rocky silhouette so sleep keeps the
  // correct broad torso and six-appendage anatomy. No frame changes occur.
  display.drawBitmap(0, 0, ROCKY_BUMP_FRAMES[0], 128, 64, WHITE);
  display.setTextSize(1);
  display.setCursor(101, 1);
  display.print(F("ZZZ"));
  drawGround();
}

void drawPace() {
  // Translate the complete reference-derived Rocky bitmap across the OLED.
  // walkX is 34..94, so the bitmap offset is -30..30: left to right and back.
  uint8_t bumpFrame = (frame / 2) % ROCKY_BUMP_FRAME_COUNT;
  int offset = walkX - 64;
  display.drawBitmap(offset, 0, ROCKY_BUMP_FRAMES[bumpFrame], 128, 64, WHITE);
}

void drawWakingUp(int cx, int top) {
  // A full-body stretch from crouched to tall. The torso is always drawn.
  uint8_t stretch = frame < 10 ? frame : 10;
  int y = 29 - stretch / 2;
  int height = 23 + stretch;
  int width = 45 + stretch / 2;
  drawConnectedTorso(cx, y, width, height);
  int shoulderY = y + 7;
  int hipY = y + height - 5;
  int handY = 37 - stretch * 2;

  // Arms rise progressively above the body.
  thickLine(cx - width / 2 + 5, shoulderY, cx - 29, handY + 7, 3);
  thickLine(cx + width / 2 - 5, shoulderY, cx + 29, handY + 7, 3);
  drawJoint(cx - 29, handY + 7);
  drawJoint(cx + 29, handY + 7);
  drawHand(cx - 35, handY, -1, true, false);
  drawHand(cx + 35, handY, 1, true, false);

  bool stillCrouched = stretch < 5;
  drawLeg(cx - width / 2 + 9, hipY, cx - 28, stillCrouched ? 53 : 47, cx - 38, 60, false);
  drawLeg(cx - 7, hipY, cx - 12, stillCrouched ? 57 : 50, cx - 18, 61, false);
  drawLeg(cx + width / 2 - 9, hipY, cx + 28, stillCrouched ? 53 : 47, cx + 38, 60, false);
  drawLeg(cx + 7, hipY, cx + 12, stillCrouched ? 57 : 50, cx + 18, 61, false);
  drawGround();
}

void drawDance(int cx, int top) {
  // Direct playback of the supplied Rocky bump reference converted to
  // 128x64 OLED frames. This keeps the actual body/limb proportions.
  uint8_t bumpFrame = (frame / 2) % ROCKY_BUMP_FRAME_COUNT;
  display.drawBitmap(0, 0, ROCKY_BUMP_FRAMES[bumpFrame], 128, 64, WHITE);
}

void drawYoyo(int cx, int top) {
  // Rocky's idle yoyo activity: one arm holds the string while the yoyo moves.
  bool down = ((frame / 3) & 1) != 0;
  int yoyoY = down ? 56 : 38;
  drawConnectedTorso(cx, top, 42, 28);
  int shoulderY = top + 7;
  int hipY = top + 22;

  thickLine(cx - 16, shoulderY, cx - 29, 24, 3);
  drawJoint(cx - 29, 24);
  drawHand(cx - 35, 18, -1, true, false);

  thickLine(cx + 16, shoulderY, cx + 29, 24, 3);
  drawJoint(cx + 29, 24);
  drawHand(cx + 35, 18, 1, false, false);
  display.drawLine(cx + 35, 18, cx + 35, yoyoY, WHITE);
  display.drawCircle(cx + 35, yoyoY, 4, WHITE);
  display.drawFastHLine(cx + 32, yoyoY, 7, BLACK);

  drawLeg(cx - 13, hipY, cx - 25, 46, cx - 35, 60, false);
  drawLeg(cx - 7, hipY, cx - 13, 49, cx - 18, 61, false);
  drawLeg(cx + 13, hipY, cx + 25, 46, cx + 35, 60, false);
  drawLeg(cx + 7, hipY, cx + 13, 49, cx + 18, 61, false);
  drawGround();
}

void drawFetch(int cx, int top) {
  // Rocky crouches, reaches toward a ball, then the ball moves back and forth.
  bool ballNear = ((frame / 4) & 1) != 0;
  int c = cx + (ballNear ? -6 : 3);
  int ballX = ballNear ? 31 : 98;
  int ballY = ballNear ? 56 : 59;
  drawConnectedTorso(c, top + 7, 40, 23);
  int shoulderY = top + 14;
  int hipY = top + 26;

  thickLine(c - 14, shoulderY, ballNear ? 34 : 44, 39, 3);
  drawJoint(ballNear ? 34 : 44, 39);
  drawHand(ballNear ? 28 : 38, 34, -1, true, false);
  thickLine(c + 14, shoulderY, c + 29, 28, 3);
  drawJoint(c + 29, 28);
  drawHand(c + 35, 23, 1, false, false);

  drawLeg(c - 12, hipY, c - 27, 51, c - 37, 61, false);
  drawLeg(c - 6, hipY, c - 13, 55, c - 18, 61, false);
  drawLeg(c + 12, hipY, c + 27, 51, c + 37, 61, false);
  drawLeg(c + 6, hipY, c + 13, 55, c + 18, 61, false);

  display.drawCircle(ballX, ballY, 3, WHITE);
  drawGround();
}

void drawFrameToBuffer() {
  display.clearDisplay();
  if (state == IDLE) drawIdle(64, 20);
  else if (state == CURIOUS) drawCurious(64, 20);
  else if (state == GREETING) drawGreeting(64, 20);
  else if (state == LISTENING) drawListening(64, 20);
  else if (state == WALK_FORWARD) drawWalking(walkX, 20, false);
  else if (state == WALK_BACK) drawWalking(walkX, 20, true);
  else if (state == PACE) drawPace();
  else if (state == COME_HERE) drawApproach(64, true);
  else if (state == GO_BACK) drawApproach(64, false);
  else if (state == SLEEPING) drawSleeping(64, 20);
  else if (state == WAKING_UP) drawWakingUp(64, 20);
  else if (state == DANCE) drawDance(64, 20);
  else if (activity == ACT_YOYO) drawYoyo(64, 20);
  else drawFetch(64, 20);
}

void drawFrame() {
  drawFrameToBuffer();
  display.display();
}

// ---------- State and personality control ----------

const char* stateName() {
  switch (state) {
    case IDLE: return "idle";
    case CURIOUS: return "curious";
    case GREETING: return "greeting";
    case LISTENING: return "listening / approaching";
    case WALK_FORWARD: return "walking forward";
    case WALK_BACK: return "walking back";
    case PACE: return "pace left-right-left";
    case COME_HERE: return "come here";
    case GO_BACK: return "go back";
    case SLEEPING: return "sleeping";
    case WAKING_UP: return "waking up";
    case DANCE: return "dance";
    case IDLE_ACTIVITY: return activity == ACT_YOYO ? "activity yoyo" : "activity fetch";
  }
  return "idle";
}

void schedulePersonalityReturn(unsigned long duration = 3000UL) {
  personalityOwnedState = true;
  returnToIdleAt = millis() + duration;
}

void setState(String value, bool fromPersonality = false) {
  value.trim();
  value.toLowerCase();

  if (value == "idle") state = IDLE;
  else if (value == "curious") state = CURIOUS;
  else if (value == "greeting" || value == "wave") state = GREETING;
  else if (value == "listening" || value == "listen") state = LISTENING;
  else if (value == "walking forward" || value == "walk forward" || value == "forward") state = WALK_FORWARD;
  else if (value == "walking back" || value == "walk back" || value == "back") state = WALK_BACK;
  else if (value == "pace" || value == "pace left right" || value == "pace left-right-left") state = PACE;
  else if (value == "come here" || value == "come") state = COME_HERE;
  else if (value == "go back" || value == "go") state = GO_BACK;
  else if (value == "sleeping" || value == "sleep") state = SLEEPING;
  else if (value == "waking up" || value == "wake" || value == "stretch") state = WAKING_UP;
  else if (value == "dance" || value == "dancing") state = DANCE;
  else {
    Serial.println(F("States: idle, curious, greeting, listening, walking forward, walking back, come here, go back, sleeping, waking up, dance"));
    return;
  }

  frame = 0;
  if (state == WALK_FORWARD) walkX = 28;
  if (state == WALK_BACK) walkX = 100;
  if (state == PACE) {
    walkX = 34;
    paceDirection = 1;
  }

  if (!fromPersonality) {
    personalityOwnedState = false;
    lastInteraction = millis();
    nextPersonalityEvent = lastInteraction + 10000UL;
  } else {
    schedulePersonalityReturn();
  }

  drawFrame();
  Serial.print(F("Rocky body state: "));
  Serial.println(stateName());
}

void setActivity(String value, bool fromPersonality = false) {
  value.trim();
  value.toLowerCase();
  if (value == "yoyo" || value == "yo-yo") activity = ACT_YOYO;
  else if (value == "fetch") activity = ACT_FETCH;
  else {
    Serial.println(F("Activities: yoyo, fetch"));
    return;
  }

  state = IDLE_ACTIVITY;
  frame = 0;
  if (!fromPersonality) {
    personalityOwnedState = false;
    lastInteraction = millis();
    nextPersonalityEvent = lastInteraction + 10000UL;
  } else {
    schedulePersonalityReturn(5000UL);
  }
  drawFrame();
  Serial.print(F("Rocky activity: "));
  Serial.println(stateName());
}

void advancePersonality() {
  if (!rockyPersonality || personalityOwnedState) return;
  if (state != IDLE) return;
  unsigned long now = millis();
  if (now < nextPersonalityEvent) return;

  // Long idle periods become small activities instead of a dead screen.
  if (personalityStep == 0) setState("curious", true);
  else if (personalityStep == 1) setState("listening", true);
  else if (personalityStep == 2) setState("greeting", true);
  else if (personalityStep == 3) setActivity("yoyo", true);
  else if (personalityStep == 4) setActivity("fetch", true);
  else if (personalityStep == 5) setState("dance", true);
  personalityStep = (personalityStep + 1) % 6;
}

void help() {
  Serial.println(F("Rocky modular personality animation"));
  Serial.println(F("Filled torso in every pose; no face or cracks."));
  Serial.println(F("  state idle"));
  Serial.println(F("  state curious"));
  Serial.println(F("  state greeting"));
  Serial.println(F("  state listening"));
  Serial.println(F("  state walking forward"));
  Serial.println(F("  state walking back"));
  Serial.println(F("  state pace"));
  Serial.println(F("  state come here"));
  Serial.println(F("  state go back"));
  Serial.println(F("  state sleeping"));
  Serial.println(F("  state waking up"));
  Serial.println(F("  state dance"));
  Serial.println(F("  activity yoyo"));
  Serial.println(F("  activity fetch"));
  Serial.println(F("  personality rocky|off"));
  Serial.println(F("  play / pause / speed 60..1000"));
}

void processCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command.startsWith("state ")) setState(command.substring(6));
  else if (command.startsWith("activity ")) setActivity(command.substring(9));
  else if (command == "play") playing = true;
  else if (command == "pause") playing = false;
  else if (command == "personality rocky") {
    rockyPersonality = true;
    lastInteraction = millis();
    nextPersonalityEvent = lastInteraction + 10000UL;
    Serial.println(F("Rocky personality ON"));
  } else if (command == "personality off") {
    rockyPersonality = false;
    personalityOwnedState = false;
    Serial.println(F("Personality automation OFF"));
  } else if (command.startsWith("speed ")) {
    int value = command.substring(6).toInt();
    if (value >= 60 && value <= 1000) frameDelay = value;
  } else if (command == "help") help();
  else if (command.length() > 0) Serial.println(F("Unknown command. Type help."));
}

// Integration adapters: the main Deskbot sketch owns the command layer and
// hardware, while this module owns Rocky's body-language state and animation.
bool bodyStateFromName(const String &value, BodyState &target) {
  String v = value;
  v.trim();
  v.toLowerCase();
  if (v == "idle") target = IDLE;
  else if (v == "curious") target = CURIOUS;
  else if (v == "greeting" || v == "wave") target = GREETING;
  else if (v == "listening" || v == "listen") target = LISTENING;
  else if (v == "walking forward" || v == "walk forward" || v == "forward") target = WALK_FORWARD;
  else if (v == "walking back" || v == "walk back" || v == "back") target = WALK_BACK;
  else if (v == "pace" || v == "pace left right" || v == "pace left-right-left") target = PACE;
  else if (v == "come here" || v == "come") target = COME_HERE;
  else if (v == "go back" || v == "go") target = GO_BACK;
  else if (v == "sleeping" || v == "sleep") target = SLEEPING;
  else if (v == "waking up" || v == "wake" || v == "stretch") target = WAKING_UP;
  else if (v == "dance" || v == "dancing") target = DANCE;
  else return false;
  return true;
}

void setIntegrationState(const String &value) {
  BodyState target = state;
  if (!bodyStateFromName(value, target)) return;
  personalityOwnedState = false;
  if (target != state) setState(value, false);
}

void renderIntegrationFrame() {
  drawFrameToBuffer();
}

void tickIntegration(unsigned long now) {
  if (!playing || now - lastFrame < frameDelay) return;
  lastFrame = now;
  if (state != SLEEPING) frame++;

  if (state == WALK_FORWARD) {
    walkX += 2;
    if (walkX > 94) walkX = 34;
  } else if (state == WALK_BACK) {
    walkX -= 2;
    if (walkX < 34) walkX = 94;
  } else if (state == PACE) {
    walkX += paceDirection * 2;
    if (walkX >= 94) {
      walkX = 94;
      paceDirection = -1;
    } else if (walkX <= 34) {
      walkX = 34;
      paceDirection = 1;
    }
  }
}

} // namespace RockyBodyLanguage
