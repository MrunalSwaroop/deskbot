#pragma once

// Procedural stoic commander. The supplied bitmap is used only as an anatomy
// template; this renderer draws the character from primitives so every state
// can move, gesture, and express personality independently.
extern Adafruit_SSD1306 display;

enum SpartanState {
  SPARTAN_GUARD,
  SPARTAN_COMMAND,
  SPARTAN_SALUTE,
  SPARTAN_LISTENING,
  SPARTAN_THINKING,
  SPARTAN_LAUGHING,
  SPARTAN_MARCH,
  SPARTAN_ATTACK,
  SPARTAN_VICTORY,
  SPARTAN_SLEEPING
};

SpartanState spartanState = SPARTAN_GUARD;
uint8_t spartanFrame = 0;
bool spartanAuto = true;

const char* spartanStateName() {
  switch (spartanState) {
    case SPARTAN_GUARD: return "guard";
    case SPARTAN_COMMAND: return "command";
    case SPARTAN_SALUTE: return "salute";
    case SPARTAN_LISTENING: return "listening";
    case SPARTAN_THINKING: return "thinking";
    case SPARTAN_LAUGHING: return "laughing";
    case SPARTAN_MARCH: return "march";
    case SPARTAN_ATTACK: return "attack";
    case SPARTAN_VICTORY: return "victory";
    case SPARTAN_SLEEPING: return "sleeping";
  }
  return "guard";
}

void spartanLine(int x1, int y1, int x2, int y2, uint8_t width = 1) {
  display.drawLine(x1, y1, x2, y2, WHITE);
  if (width >= 2) display.drawLine(x1, y1 + 1, x2, y2 + 1, WHITE);
  if (width >= 3) display.drawLine(x1, y1 - 1, x2, y2 - 1, WHITE);
}

void spartanJoint(int x, int y) {
  display.fillRect(x - 2, y - 1, 5, 3, WHITE);
  display.drawPixel(x, y, BLACK);
}

void drawSpartanGround() { display.drawFastHLine(7, 63, 114, WHITE); }

void drawSpartanHair(int cx, int top) {
  // Separated curl arcs keep the commander readable at OLED scale.
  display.drawRoundRect(cx - 18, top + 1, 36, 30, 9, WHITE);
  display.drawCircle(cx - 12, top + 3, 3, WHITE);
  display.drawCircle(cx - 5, top + 1, 3, WHITE);
  display.drawCircle(cx + 3, top + 1, 3, WHITE);
  display.drawCircle(cx + 11, top + 3, 3, WHITE);
  display.drawLine(cx - 16, top + 8, cx - 21, top + 15, WHITE);
  display.drawLine(cx + 16, top + 8, cx + 21, top + 15, WHITE);
}

void drawSpartanFace(int cx, int top, bool sleeping, bool turned = false) {
  drawSpartanHair(cx, top);
  // Open face: the black interior separates features from hair and armor.
  display.fillRoundRect(cx - 14, top + 8, 28, 25, 7, WHITE);
  display.fillRoundRect(cx - 11, top + 11, 22, 20, 6, BLACK);

  display.drawLine(cx - 8, top + 15, cx - 2, top + 14, WHITE);
  display.drawLine(cx + 2, top + 14, cx + 8, top + 15, WHITE);
  if (sleeping) {
    display.drawFastHLine(cx - 7, top + 20, 5, WHITE);
    display.drawFastHLine(cx + 2, top + 20, 5, WHITE);
  } else {
    display.drawFastHLine(cx - 7, top + 20, 5, WHITE);
    display.drawFastHLine(cx + 2, top + 20, 5, WHITE);
    display.drawPixel(cx + (turned ? 4 : -4), top + 20, WHITE);
  }
  display.drawFastVLine(cx, top + 17, 7, WHITE);
  display.drawLine(cx, top + 24, cx + 3, top + 25, WHITE);

  // Beard outline with visible moustache and lower point.
  display.fillTriangle(cx - 12, top + 25, cx + 12, top + 25, cx, top + 41, WHITE);
  display.drawLine(cx - 7, top + 29, cx - 2, top + 36, BLACK);
  display.drawLine(cx + 7, top + 29, cx + 2, top + 36, BLACK);
  display.drawFastHLine(cx - 5, top + 31, 10, BLACK);
  display.drawFastHLine(cx - 3, top + 36, 6, BLACK);
}

void drawSpartanChest(int cx, int top, int shift = 0) {
  // Broad draped shoulders and a simple chest medallion from the template.
  display.drawRoundRect(cx - 24 + shift, top + 2, 48, 27, 6, WHITE);
  display.drawLine(cx - 23 + shift, top + 7, cx - 34 + shift, top + 25, WHITE);
  display.drawLine(cx + 23 + shift, top + 7, cx + 34 + shift, top + 25, WHITE);
  display.drawLine(cx - 17 + shift, top + 3, cx - 8 + shift, top + 29, WHITE);
  display.drawLine(cx + 17 + shift, top + 3, cx + 8 + shift, top + 29, WHITE);
  display.drawFastVLine(cx + shift, top + 8, 19, WHITE);
  display.drawCircle(cx + shift, top + 10, 4, WHITE);
  display.drawPixel(cx + shift, top + 10, BLACK);
  display.drawFastHLine(cx - 12 + shift, top + 20, 24, WHITE);
}

void drawSpartanPalm(int x, int y, int direction, bool pointing = false, bool open = true) {
  // Palm plus three short fingers. This reads as a hand, not a stick figure.
  display.fillRoundRect(x - 4, y - 4, 8, 8, 2, WHITE);
  display.drawPixel(x, y, BLACK);
  if (pointing) {
    spartanLine(x + direction * 2, y - 1, x + direction * 11, y - 4, 2);
    display.drawPixel(x + direction * 12, y - 4, WHITE);
    spartanLine(x + direction * 2, y + 1, x + direction * 7, y + 5, 1);
  } else if (open) {
    spartanLine(x + direction * 2, y - 2, x + direction * 8, y - 8, 1);
    spartanLine(x + direction * 3, y, x + direction * 10, y - 3, 1);
    spartanLine(x + direction * 2, y + 2, x + direction * 8, y + 4, 1);
  } else {
    display.fillRect(x + direction * 2 - 2, y - 2, 5, 5, WHITE);
  }
}

void drawSpartanArm(int shoulderX, int shoulderY, int elbowX, int elbowY,
                    int handX, int handY, int direction,
                    bool pointing = false, bool open = true) {
  spartanLine(shoulderX, shoulderY, elbowX, elbowY, 2);
  spartanJoint(elbowX, elbowY);
  spartanLine(elbowX, elbowY, handX, handY, 2);
  drawSpartanPalm(handX, handY, direction, pointing, open);
}

void drawSpartanSpear(int x, int y, int endX, int endY) {
  spartanLine(x, y, endX, endY, 1);
  display.drawLine(endX, endY, endX - 3, endY + 6, WHITE);
  display.drawLine(endX, endY, endX + 3, endY + 6, WHITE);
  display.drawLine(endX, endY, endX, endY + 7, WHITE);
}

void drawSpartanGuard() {
  int cx = 66;
  drawSpartanFace(cx, 2, false);
  drawSpartanChest(cx, 39);
  drawSpartanArm(52, 47, 39, 52, 28, 56, -1, false, true);
  drawSpartanArm(80, 47, 93, 52, 104, 56, 1, false, true);
  drawSpartanGround();
}

void drawSpartanCommand() {
  int cx = 66;
  drawSpartanFace(cx, 2, false);
  drawSpartanChest(cx, 39);
  // Signature template gesture, redrawn with a palm and separated fingers.
  drawSpartanArm(52, 47, 38, 39, 19, 29, -1, true, true);
  drawSpartanArm(80, 47, 93, 52, 104, 56, 1, false, true);
  drawSpartanGround();
}

void drawSpartanSalute() {
  int cx = 66;
  drawSpartanFace(cx, 2, false);
  drawSpartanChest(cx, 39);
  drawSpartanArm(52, 47, 45, 35, 53, 16, 1, false, false);
  drawSpartanArm(80, 47, 93, 52, 104, 56, 1, false, true);
  drawSpartanGround();
}

void drawSpartanListening() {
  int cx = ((spartanFrame / 6) & 1) ? 63 : 66;
  drawSpartanFace(cx, 3, false, true);
  drawSpartanChest(cx, 39, -2);
  // One cupped hand beside the ear; the other stays open and grounded.
  drawSpartanArm(cx - 13, 47, cx - 29, 33, cx - 22, 20, 1, false, true);
  drawSpartanArm(cx + 13, 47, cx + 27, 54, cx + 38, 57, 1, false, true);
  display.drawCircle(cx - 15, 13, 5, WHITE);
  display.drawCircle(cx - 15, 13, 9, WHITE);
  display.fillCircle(cx - 15, 13, 3, BLACK);
  drawSpartanGround();
}

void drawSpartanThinking() {
  int cx = 66;
  drawSpartanFace(cx, 2, false);
  drawSpartanChest(cx, 39);
  // Fist rests under beard while the opposite hand remains open.
  drawSpartanArm(52, 47, 45, 37, 60, 34, 1, false, false);
  drawSpartanArm(80, 47, 93, 52, 104, 56, 1, false, true);
  display.drawCircle(45, 19, 3, WHITE);
  display.drawCircle(54, 14, 2, WHITE);
  drawSpartanGround();
}

void drawSpartanLaughing() {
  // A rare, restrained commander laugh: closed eyes, a small open mouth,
  // subtle shoulder bounce, and one palm resting against the chest.
  bool bounce = ((spartanFrame / 3) & 1) != 0;
  int cx = 66;
  int y = bounce ? 3 : 2;
  drawSpartanFace(cx, y, true);
  display.fillRoundRect(cx - 5, y + 25, 10, 7, 3, BLACK);
  display.drawFastHLine(cx - 3, y + 28, 6, WHITE);
  drawSpartanChest(cx, 39 + (bounce ? 1 : 0));
  drawSpartanArm(52, 47, 45, 40, 58, 34, 1, false, false);
  drawSpartanArm(80, 47, 93, 52, 104, 56, 1, false, true);
  drawSpartanGround();
}

void drawSpartanMarch() {
  bool step = ((spartanFrame / 4) & 1) != 0;
  int cx = step ? 62 : 66;
  drawSpartanFace(cx, 2, false, step);
  drawSpartanChest(cx, 39, step ? 2 : 0);
  drawSpartanArm(cx - 13, 47, cx - 29, step ? 41 : 53,
                 cx - 43, step ? 31 : 58, -1, true, true);
  drawSpartanArm(cx + 13, 47, cx + 29, step ? 54 : 41,
                 cx + 42, step ? 59 : 31, 1, false, true);
  spartanLine(cx - 7, 59, cx - 17, step ? 50 : 63, 2);
  spartanLine(cx + 7, 59, cx + 18, step ? 63 : 50, 2);
  drawSpartanGround();
}

void drawSpartanAttack() {
  bool thrust = ((spartanFrame / 4) & 1) != 0;
  int cx = 55;
  drawSpartanFace(cx, 4, false, true);
  drawSpartanChest(cx, 41, 2);
  drawSpartanArm(cx + 13, 48, 78, 43, thrust ? 107 : 96, 36, 1, true, true);
  drawSpartanArm(cx - 13, 48, 36, 54, 24, 59, -1, false, false);
  drawSpartanSpear(78, 43, thrust ? 127 : 115, 36);
  drawSpartanGround();
}

void drawSpartanVictory() {
  bool lift = ((spartanFrame / 6) & 1) != 0;
  int cx = 66;
  drawSpartanFace(cx, lift ? 1 : 2, false);
  drawSpartanChest(cx, 39);
  drawSpartanArm(52, 47, 38, 28, 24, 16, -1, false, true);
  drawSpartanArm(80, 47, 94, 28, 106, 16, 1, false, true);
  drawSpartanGround();
}

void drawSpartanSleeping() {
  drawSpartanFace(50, 28, true);
  display.fillRoundRect(28, 51, 57, 7, 3, WHITE);
  display.drawFastHLine(38, 54, 17, BLACK);
  display.setTextSize(1);
  display.setCursor(5, 2);
  display.print(F("Z"));
  display.setCursor(15, 0);
  display.print(F("Z"));
  drawSpartanGround();
}

void drawSpartan() {
  switch (spartanState) {
    case SPARTAN_GUARD: drawSpartanGuard(); break;
    case SPARTAN_COMMAND: drawSpartanCommand(); break;
    case SPARTAN_SALUTE: drawSpartanSalute(); break;
    case SPARTAN_LISTENING: drawSpartanListening(); break;
    case SPARTAN_THINKING: drawSpartanThinking(); break;
    case SPARTAN_LAUGHING: drawSpartanLaughing(); break;
    case SPARTAN_MARCH: drawSpartanMarch(); break;
    case SPARTAN_ATTACK: drawSpartanAttack(); break;
    case SPARTAN_VICTORY: drawSpartanVictory(); break;
    case SPARTAN_SLEEPING: drawSpartanSleeping(); break;
  }
}

void setSpartanState(SpartanState next) {
  spartanState = next;
  spartanFrame = 0;
}

bool setSpartanStateByName(String name) {
  name.trim();
  name.toLowerCase();
  if (name == "guard" || name == "idle") setSpartanState(SPARTAN_GUARD);
  else if (name == "command" || name == "point") setSpartanState(SPARTAN_COMMAND);
  else if (name == "salute" || name == "hail") setSpartanState(SPARTAN_SALUTE);
  else if (name == "listening" || name == "listen") setSpartanState(SPARTAN_LISTENING);
  else if (name == "thinking" || name == "think") setSpartanState(SPARTAN_THINKING);
  else if (name == "laughing" || name == "laugh") setSpartanState(SPARTAN_LAUGHING);
  else if (name == "march" || name == "walking") setSpartanState(SPARTAN_MARCH);
  else if (name == "attack" || name == "thrust") setSpartanState(SPARTAN_ATTACK);
  else if (name == "victory" || name == "win") setSpartanState(SPARTAN_VICTORY);
  else if (name == "sleeping" || name == "sleep") setSpartanState(SPARTAN_SLEEPING);
  else return false;
  return true;
}

void printSpartanHelp() {
  Serial.println(F("Spartan states: guard, command, salute, listening, thinking, laughing, march, attack, victory, sleeping"));
  Serial.println(F("Commands: spartan <state>, spartan auto on|off"));
}

void advanceSpartanPersonality() {
  if (!spartanAuto || spartanState == SPARTAN_SLEEPING) return;
  if ((spartanFrame % 75) != 0 || spartanFrame == 0) return;
  // Laughing is occasional and social, not a constant random idle loop.
  if (random(0, 8) == 0) {
    setSpartanState(SPARTAN_LAUGHING);
    return;
  }
  static uint8_t step = 0;
  const SpartanState sequence[] = {
    SPARTAN_GUARD, SPARTAN_COMMAND, SPARTAN_LISTENING,
    SPARTAN_THINKING, SPARTAN_MARCH, SPARTAN_VICTORY
  };
  setSpartanState(sequence[step]);
  step = (step + 1) % (sizeof(sequence) / sizeof(sequence[0]));
}
