#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <WiFiS3.h>
#include "arduino_secrets.h"
#include "eyes.h"

// Rocky-Wall-E pan-only Wi-Fi desk robot.
// The UNO R4 owns the OLED face, optional pan servo, and a local HTTP API.
// Motors are deliberately not included in this stage.
// Type either a simple word (happy) or a full command (EYE happy).
//
// The face uses the original RobotEyes eye sprites plus simple drawn features:
// eyebrows, cheeks, and a mouth. It is designed for a 128x64 SSD1306 OLED.

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Servo panServo;
WiFiServer server(80);

const uint8_t PAN_SERVO_PIN = 3;
const int PAN_MIN = 60;
const int PAN_CENTER = 90;
const int PAN_MAX = 120;
int panAngle = PAN_CENTER;
int wifiStatus = WL_IDLE_STATUS;

enum FaceMode {
  FACE_IDLE,
  FACE_HAPPY,
  FACE_LISTEN,
  FACE_SPEAK,
  FACE_SLEEP,
  FACE_SURPRISE,
  FACE_SAD,
  FACE_CURIOUS,
  FACE_WORRIED,
  FACE_LEFT,
  FACE_RIGHT
};

FaceMode faceMode = FACE_IDLE;
int eyeMood = 1;       // 0..5 entries from eyes.h
int eyePosition = 16;  // 0 = left, 16 = centre, 32 = right
int eyeJitter = 0;
int mouthOverride = 0; // 0 = expression default, 1 = smile, 2 = open, 3 = flat
bool personalityOn = true;
bool blinking = false;
bool mouthOpen = false;
unsigned long nextBlinkAt = 0;
unsigned long blinkEndsAt = 0;
unsigned long nextIdleGlanceAt = 0;
unsigned long nextMouthFrameAt = 0;
String inputLine;

void connectWiFi();
void handleWiFiClient();
void movePan(int newAngle);
String urlDecode(String value);

void setup() {
  Serial.begin(115200);
  inputLine.reserve(64);

  panServo.attach(PAN_SERVO_PIN);
  panServo.write(panAngle);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED ERROR: check VCC, GND, SDA, SCL, address 0x3C/0x3D");
    while (true) {
      delay(1000);
    }
  }

  randomSeed(analogRead(A0));
  showBootGreeting();
  setFace("idle");
  connectWiFi();

  Serial.println();
  Serial.println("Rocky pan-only Wi-Fi face ready.");
  Serial.println("Try: happy, listen, speak, sleep, surprise, sad, curious, worried, left, right, idle");
  Serial.println("Pan: pan 60..120, look left, look right, look center");
  Serial.println("You can also type: EYE happy");
  Serial.println("Type HELP for all commands.");
}

void loop() {
  readSerialCommands();
  handleWiFiClient();
  updatePersonality();
}

void readSerialCommands() {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (inputLine.length() > 0) {
        processCommand(inputLine);
        inputLine = "";
      }
    } else if (inputLine.length() < 63) {
      inputLine += c;
    }
  }
}

void processCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command == "help") {
    Serial.println("Commands: happy, listen, speak, sleep, surprise, sad, curious, worried, left, right, idle");
    Serial.println("Pan: pan 60..120, look left, look right, look center");
    Serial.println("Also: personality on, personality off, mouth smile, mouth open, mouth flat, clear");
    return;
  }

  if (command.startsWith("eye ")) {
    command = command.substring(4);
    command.trim();
  }

  if (command == "personality on") {
    personalityOn = true;
    schedulePersonality();
    Serial.println("OK personality on");
    return;
  }

  if (command == "personality off") {
    personalityOn = false;
    blinking = false;
    setFace("idle");
    Serial.println("OK personality off; idle face selected");
    return;
  }

  if (command == "mouth smile" || command == "mouth open" || command == "mouth flat") {
    mouthOverride = command == "mouth smile" ? 1 : (command == "mouth open" ? 2 : 3);
    drawFace();
    Serial.print("OK ");
    Serial.println(command);
    return;
  }

  if (command == "look left") {
    movePan(PAN_MIN);
    setFace("left");
    Serial.println("OK looking left");
    return;
  }

  if (command == "look right") {
    movePan(PAN_MAX);
    setFace("right");
    Serial.println("OK looking right");
    return;
  }

  if (command == "look center" || command == "pan center") {
    movePan(PAN_CENTER);
    setFace("idle");
    Serial.println("OK looking center");
    return;
  }

  if (command.startsWith("pan ")) {
    int value = constrain(command.substring(4).toInt(), PAN_MIN, PAN_MAX);
    movePan(value);
    Serial.print("OK pan ");
    Serial.println(value);
    return;
  }

  if (command == "clear") {
    display.clearDisplay();
    display.display();
    Serial.println("OK display cleared");
    return;
  }

  if (isFaceName(command)) {
    setFace(command);
    Serial.print("OK showing ");
    Serial.println(command);
    return;
  }

  Serial.println("ERR unknown command; type HELP");
}

bool isFaceName(String face) {
  return face == "idle" || face == "happy" || face == "listen" ||
         face == "speak" || face == "sleep" || face == "surprise" ||
         face == "sad" || face == "curious" || face == "worried" ||
         face == "left" || face == "right";
}

void setFace(String face) {
  blinking = false;
  mouthOpen = false;
  eyeJitter = 0;
  mouthOverride = 0;

  if (face == "idle") {
    faceMode = FACE_IDLE;
    eyeMood = 1;
    eyePosition = 16;
  } else if (face == "happy") {
    faceMode = FACE_HAPPY;
    eyeMood = 0;
    eyePosition = 16;
  } else if (face == "listen") {
    faceMode = FACE_LISTEN;
    eyeMood = 3;
    eyePosition = 16;
  } else if (face == "speak") {
    faceMode = FACE_SPEAK;
    eyeMood = 4;
    eyePosition = 16;
  } else if (face == "sleep") {
    faceMode = FACE_SLEEP;
    eyeMood = 2;
    eyePosition = 16;
  } else if (face == "surprise") {
    faceMode = FACE_SURPRISE;
    eyeMood = 5;
    eyePosition = 16;
  } else if (face == "sad") {
    faceMode = FACE_SAD;
    eyeMood = 4;
    eyePosition = 16;
  } else if (face == "curious") {
    faceMode = FACE_CURIOUS;
    eyeMood = 1;
    eyePosition = 16;
  } else if (face == "worried") {
    faceMode = FACE_WORRIED;
    eyeMood = 5;
    eyePosition = 16;
  } else if (face == "left") {
    faceMode = FACE_LEFT;
    eyeMood = 1;
    eyePosition = 0;
  } else if (face == "right") {
    faceMode = FACE_RIGHT;
    eyeMood = 1;
    eyePosition = 32;
  }

  schedulePersonality();
  drawFace();
}

void schedulePersonality() {
  unsigned long now = millis();
  nextBlinkAt = now + random(3500, 7500);
  nextIdleGlanceAt = now + random(2500, 6000);
  nextMouthFrameAt = now + 180;
}

void movePan(int newAngle) {
  panAngle = constrain(newAngle, PAN_MIN, PAN_MAX);
  panServo.write(panAngle);
}

void connectWiFi() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WIFI ERROR: WiFi module not detected");
    return;
  }

  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(SECRET_SSID);
  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 20000) {
    wifiStatus = WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(3000);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiStatus = WL_CONNECTED;
    server.begin();
    Serial.print("WIFI OK IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("HTTP: GET http://ROBOT_IP/health or /cmd/happy");
  } else {
    Serial.println("WIFI ERROR: continuing with USB serial only");
  }
}

void handleWiFiClient() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client = server.available();
  if (!client) return;

  String requestLine = client.readStringUntil('\n');
  requestLine.trim();

  // Drain the remainder of the HTTP headers so the client can close cleanly.
  while (client.connected()) {
    String header = client.readStringUntil('\n');
    if (header == "\r" || header.length() == 0) break;
  }

  String body = "ROCKY_WALLE_OK";
  int getStart = requestLine.indexOf("GET ");
  int pathEnd = requestLine.indexOf(' ', getStart + 4);
  if (getStart >= 0 && pathEnd > getStart) {
    String path = requestLine.substring(getStart + 4, pathEnd);
    if (path == "/health") {
      body = "OK IP=" + WiFi.localIP().toString() + " PAN=" + String(panAngle);
    } else if (path.startsWith("/cmd/")) {
      String command = urlDecode(path.substring(5));
      processCommand(command);
      body = "OK command=" + command;
    } else {
      body = "Use /health or /cmd/happy, /cmd/look%20left, /cmd/pan%2090";
    }
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println(body);
  delay(1);
  client.stop();
}

String urlDecode(String value) {
  value.replace("+", " ");
  for (int i = 0; i + 2 < value.length(); i++) {
    if (value[i] == '%') {
      char high = value[i + 1];
      char low = value[i + 2];
      int highValue = high <= '9' ? high - '0' : high >= 'a' ? high - 'a' + 10 : high - 'A' + 10;
      int lowValue = low <= '9' ? low - '0' : low >= 'a' ? low - 'a' + 10 : low - 'A' + 10;
      value.setCharAt(i, static_cast<char>((highValue << 4) | lowValue));
      value.remove(i + 1, 2);
    }
  }
  return value;
}

void updatePersonality() {
  if (!personalityOn) return;

  unsigned long now = millis();

  if (blinking) {
    if (now >= blinkEndsAt) {
      blinking = false;
      drawFace();
    }
    return;
  }

  if (now >= nextBlinkAt) {
    // Avoid interrupting a sleep face; closed eyes already communicate sleep.
    if (faceMode != FACE_SLEEP) {
      blinking = true;
      blinkEndsAt = now + 110;
      drawFace();
    }
    nextBlinkAt = now + random(3500, 7500);
    return;
  }

  // Animate the mouth gently while the robot is speaking.
  if (faceMode == FACE_SPEAK && now >= nextMouthFrameAt) {
    mouthOpen = !mouthOpen;
    nextMouthFrameAt = now + random(130, 260);
    drawFace();
    return;
  }

  // The neutral face makes occasional tiny glances. Explicit moods stay stable
  // so the user can inspect them on the bench.
  if (faceMode == FACE_IDLE && now >= nextIdleGlanceAt) {
    eyePosition = random(0, 3) == 0 ? 0 : (random(0, 3) == 1 ? 32 : 16);
    nextIdleGlanceAt = now + random(1800, 4500);
    drawFace();
  }
}

void drawFace() {
  display.clearDisplay();

  int leftX = constrain(eyePosition + eyeJitter, 0, 96);
  int rightX = constrain(64 + eyePosition + eyeJitter, 32, 127);

  if (blinking || faceMode == FACE_SLEEP) {
    display.drawBitmap(leftX, 16, eye0, 32, 32, SSD1306_WHITE);
    display.drawBitmap(rightX, 16, eye0, 32, 32, SSD1306_WHITE);
  } else {
    const unsigned char *leftEye = peyes[eyeMood][0][0];
    const unsigned char *rightEye = peyes[eyeMood][0][1];
    display.drawBitmap(leftX, 8, leftEye, 32, 32, SSD1306_WHITE);
    display.drawBitmap(rightX, 8, rightEye, 32, 32, SSD1306_WHITE);
  }

  drawEyebrows();
  drawCheeks();
  drawMouth();
  display.display();
}

void drawEyebrows() {
  switch (faceMode) {
    case FACE_HAPPY:
      display.drawLine(20, 4, 36, 1, SSD1306_WHITE);
      display.drawLine(92, 1, 108, 4, SSD1306_WHITE);
      break;
    case FACE_SAD:
      display.drawLine(20, 1, 36, 5, SSD1306_WHITE);
      display.drawLine(92, 5, 108, 1, SSD1306_WHITE);
      break;
    case FACE_WORRIED:
      display.drawLine(20, 2, 36, 5, SSD1306_WHITE);
      display.drawLine(92, 5, 108, 2, SSD1306_WHITE);
      break;
    case FACE_CURIOUS:
      display.drawLine(20, 3, 36, 1, SSD1306_WHITE);
      display.drawLine(92, 1, 108, 1, SSD1306_WHITE);
      break;
    default:
      break;
  }
}

void drawCheeks() {
  if (faceMode == FACE_HAPPY || faceMode == FACE_LISTEN) {
    // Small dotted cheeks, kept inside the 128x64 display area.
    display.drawPixel(10, 48, SSD1306_WHITE);
    display.drawPixel(13, 49, SSD1306_WHITE);
    display.drawPixel(114, 48, SSD1306_WHITE);
    display.drawPixel(117, 49, SSD1306_WHITE);
  }
}

void drawMouth() {
  // Mouth area is below the 32x32 eye sprites.
  if (mouthOverride == 1) {
    display.drawLine(52, 49, 56, 52, SSD1306_WHITE);
    display.drawLine(56, 52, 64, 54, SSD1306_WHITE);
    display.drawLine(64, 54, 72, 52, SSD1306_WHITE);
    display.drawLine(72, 52, 76, 49, SSD1306_WHITE);
    return;
  }
  if (mouthOverride == 2) {
    display.fillRoundRect(54, 47, 20, 10, 4, SSD1306_WHITE);
    return;
  }
  if (mouthOverride == 3) {
    display.drawLine(55, 51, 73, 51, SSD1306_WHITE);
    return;
  }

  switch (faceMode) {
    case FACE_HAPPY:
      // Gentle smile.
      display.drawLine(52, 49, 56, 52, SSD1306_WHITE);
      display.drawLine(56, 52, 64, 54, SSD1306_WHITE);
      display.drawLine(64, 54, 72, 52, SSD1306_WHITE);
      display.drawLine(72, 52, 76, 49, SSD1306_WHITE);
      break;
    case FACE_LISTEN:
      // Small attentive oval.
      display.drawRoundRect(59, 48, 10, 8, 3, SSD1306_WHITE);
      break;
    case FACE_SPEAK:
      if (mouthOpen) {
        display.fillRoundRect(54, 47, 20, 10, 4, SSD1306_WHITE);
        display.drawLine(59, 52, 69, 52, SSD1306_BLACK);
      } else {
        display.drawLine(55, 51, 73, 51, SSD1306_WHITE);
      }
      break;
    case FACE_SLEEP:
      display.drawLine(56, 51, 72, 51, SSD1306_WHITE);
      display.drawLine(60, 54, 68, 54, SSD1306_WHITE);
      break;
    case FACE_SURPRISE:
      display.drawCircle(64, 51, 5, SSD1306_WHITE);
      break;
    case FACE_SAD:
      display.drawLine(55, 54, 60, 51, SSD1306_WHITE);
      display.drawLine(60, 51, 68, 51, SSD1306_WHITE);
      display.drawLine(68, 51, 73, 54, SSD1306_WHITE);
      break;
    case FACE_WORRIED:
      display.drawLine(55, 52, 60, 50, SSD1306_WHITE);
      display.drawLine(60, 50, 68, 50, SSD1306_WHITE);
      display.drawLine(68, 50, 73, 52, SSD1306_WHITE);
      break;
    case FACE_CURIOUS:
      display.drawLine(57, 51, 71, 51, SSD1306_WHITE);
      display.drawPixel(74, 50, SSD1306_WHITE);
      break;
    default:
      // Quiet, neutral desk-buddy mouth.
      display.drawLine(58, 51, 70, 51, SSD1306_WHITE);
      break;
  }
}

void showBootGreeting() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(25, 16);
  display.println("HELLO, HUMAN");
  display.setCursor(31, 30);
  display.println("ROCKY READY");
  display.setCursor(28, 48);
  display.println("I AM CALM");
  display.display();
  delay(1400);
}
