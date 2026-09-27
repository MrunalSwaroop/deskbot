#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <WiFiS3.h>
#include <time.h>
#include "WiFiSSLClient.h"
#include "arduino_secrets.h"
#include "eyes.h"

char wifiSsid[] = SECRET_SSID;
char wifiPass[] = SECRET_PASS;

// Rocky-Wall-E Wi-Fi desk robot with one-axis pan and optional L293D tracks.
// The UNO R4 owns the OLED face, optional pan servo, L293D motor outputs,
// and a local HTTP API. The normal RA4M1 + ESP32 bridge + WiFiS3 architecture
// is preserved; the onboard ESP32 firmware is not reflashed.
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
WiFiServer fallbackServer(8080);
WiFiSSLClient llmClient;

const uint8_t PAN_SERVO_PIN = 3;
// L293D: enable pins must be PWM-capable. Change these only if your wiring differs.
const uint8_t LEFT_EN_PIN = 5;
const uint8_t LEFT_IN1_PIN = 6;
const uint8_t LEFT_IN2_PIN = 7;
const uint8_t RIGHT_EN_PIN = 9;
const uint8_t RIGHT_IN1_PIN = 10;
const uint8_t RIGHT_IN2_PIN = 11;
// External RGB LED: one 220–330 ohm resistor per colour channel.
const uint8_t RGB_R_PIN = 2;
const uint8_t RGB_G_PIN = 4;
const uint8_t RGB_B_PIN = 8;
const bool RGB_COMMON_ANODE = false;
const int PAN_MIN = 0;
const int PAN_CENTER = 90;
const int PAN_MAX = 180;
int panAngle = PAN_CENTER;
int wifiStatus = WL_IDLE_STATUS;
bool wifiConnected = false;
unsigned long wifiAttemptStartedAt = 0;
unsigned long nextWifiAttemptAt = 0;
uint8_t wifiAttemptCount = 0;
bool motorsEnabled = false;
int motorSpeed = 150;
unsigned long motorMotionEndsAt = 0;
String speakingText = "...";
unsigned long speakingEndsAt = 0;
bool checkInsEnabled = true;
unsigned long nextCheckInAt = 0;
unsigned long checkInListeningUntil = 0;
bool checkInWaitingForReply = false;
bool vibeEnabled = false;
uint8_t vibeLevel = 0;
uint8_t vibeStep = 0;
unsigned long nextVibeAt = 0;
bool clockReady = false;
unsigned long clockEpochBase = 0;
unsigned long clockMillisBase = 0;
unsigned long nextClockSyncAt = 0;
String weatherSummary = "Set weather from dashboard";
bool headFollowEyes = false;
bool waterReminderEnabled = true;
unsigned long waterIntervalMs = 45UL * 60UL * 1000UL;
unsigned long nextWaterReminderAt = 0;
bool pomodoroActive = false;
bool pomodoroBreak = false;
unsigned long pomodoroEndsAt = 0;
uint16_t pomodoroSessions = 0;
uint16_t waterReminders = 0;
uint16_t checkInsToday = 0;
uint16_t questionsToday = 0;
String lastDayActivity = "Booted";
unsigned long dayStartedAt = 0;
uint8_t ledPattern = 0;
unsigned long nextLedAt = 0;
bool ledState = false;
bool infoCardActive = false;
uint8_t infoCardMode = 0;
unsigned long infoCardEndsAt = 0;
unsigned long nextIdleInfoAt = 0;
bool identityRevealActive = false;
uint8_t identityRevealMode = 0;
unsigned long identityRevealEndsAt = 0;
#ifndef LLM_HOST
#define LLM_HOST "api.openai.com"
#endif
#ifndef LLM_PATH
#define LLM_PATH "/v1/chat/completions"
#endif
#ifndef LLM_MODEL
#define LLM_MODEL "gpt-4o-mini"
#endif
#ifndef LLM_API_KEY
#define LLM_API_KEY ""
#endif
#ifndef TIMEZONE_OFFSET_SECONDS
#define TIMEZONE_OFFSET_SECONDS 19800
#endif

enum FaceMode {
  FACE_IDLE,
  FACE_HAPPY,
  FACE_LISTEN,
  FACE_SPEAK,
  FACE_THINKING,
  FACE_SLEEP,
  FACE_SURPRISE,
  FACE_SAD,
  FACE_CURIOUS,
  FACE_WORRIED,
  FACE_NOTIFICATION,
  FACE_LEFT,
  FACE_RIGHT,
  FACE_LAUGH
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
unsigned long nextCharacterMotionAt = 0;
uint8_t characterMotionStep = 0;
String inputLine;
String personalityMode = "calm";
String pendingNotification = "";
String lastLLMReply = "";
unsigned long llmReplySequence = 0;
bool danceActive = false;
int danceStep = 0;
unsigned long nextDanceAt = 0;
unsigned long notificationEndsAt = 0;
enum InteractionState {
  STATE_IDLE,
  STATE_LISTENING,
  STATE_THINKING,
  STATE_SPEAKING,
  STATE_HAPPY,
  STATE_DANCE,
  STATE_NOTIFICATION,
  STATE_ERROR,
  STATE_SLEEP
};
InteractionState interactionState = STATE_IDLE;

void connectWiFi();
void maintainWiFi();
bool hasValidLocalIP();
void drawWifiIcon();
void handleWiFiClient();
void handleWiFiClientOn(WiFiServer &activeServer);
void sendDashboard(WiFiClient &client);
void sendHttpResponse(WiFiClient &client, const String &contentType, const String &body);
String buildStatusJson();
void movePan(int newAngle);
void setupMotors();
void stopMotors();
void setMotor(int left, int right);
void driveMotors(int left, int right, unsigned long durationMs);
void updateMotorSafety();
String urlDecode(String value);
void setState(String state);
void startDance();
void stopDance();
void updateDance();
void enqueueNotification(String message);
void askLLM(String prompt);
String jsonEscape(String value);
String findLLMAction(String response);
String extractLLMReplyText(String response);
void setSpeakingText(String text);
void updateVibe();
void updateClock();
String currentTimeText();
void updateStatusLED();
void setRgb(uint8_t r, uint8_t g, uint8_t b);
void setPersonalityRgb();
void drawRgbStatus();
void updateReminders();
void syncHeadToEyes();
void startPomodoro();
void stopPomodoro();
void recordActivity(String activity);
void startCheckIn();
void scheduleNextCheckIn();
String buildCheckInPrompt();
void showBootGreeting();
void readSerialCommands();
void processCommand(String command);
bool isFaceName(String face);
void setFace(String face);
void schedulePersonality();
void updatePersonality();
void drawFace();
void drawPersonalityIcon();
void drawIdentityCard();
void showIdentityReveal(String mode);
void updateIdentityReveal();
void drawEyebrows();
void drawCheeks();
void drawMouth();
void drawPersonalityOverlay();
void updateCharacterAnimation();
void drawInfoCard(uint8_t mode);
void showInfoCard(uint8_t mode, unsigned long durationMs);
void updateInfoCard();
int personalityEyeMood();
void applyPersonalityFaceStyle();

void setup() {
  Serial.begin(115200);
  inputLine.reserve(64);
  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);
  setRgb(0, 0, 0);
  dayStartedAt = millis();
  nextIdleInfoAt = millis() + 45000UL;

  setupMotors();
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
  setState("idle");
  scheduleNextCheckIn();
  nextWaterReminderAt = millis() + waterIntervalMs;
  connectWiFi();

  Serial.println();
  Serial.println("Rocky desk buddy ready; existing UNO R4 WiFi architecture preserved.");
  Serial.println("States: idle, listening, thinking, speaking, happy, dance, notification, error, sleep");
  Serial.println("Modes: calm, musical, engineer, quiet, spartan (mode changes eyes and LLM style)");
  Serial.println("Motors are OFF. Test: motors on, motors test, motors off");
  Serial.println("Try: state listening, state thinking, state speaking, dance rocky");
}

void loop() {
  readSerialCommands();
  maintainWiFi();
  handleWiFiClient();
  updatePersonality();
  updateCharacterAnimation();
  updateIdentityReveal();
  updateDance();
  updateVibe();
  updateClock();
  updateReminders();
  updateInfoCard();
  syncHeadToEyes();
  updateStatusLED();
  updateMotorSafety();
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
    Serial.println("States: state idle|listening|thinking|speaking|happy|dance|notification|error|sleep");
    Serial.println("Modes: mode calm|musical|engineer|quiet|spartan; dance rocky; stop; notify text");
    Serial.println("Modes change the LLM response style; they do not change Wi-Fi or motor power.");
    Serial.println("Motors: motors on|off|test; drive forward|backward; turn left|right");
    Serial.println("Wi-Fi: status; the OLED top-right icon is connected or crossed-out");
    Serial.println("Notifications: notify text; read notifications");
    Serial.println("LLM direct: llm test; ask your question (OpenAI-compatible key only)");
    Serial.println("Manus: use the laptop relay POST /manus/ask with MANUS_API_KEY in its environment");
    Serial.println("Check-ins: checkins on|off; checkin now");
    Serial.println("Vibe: vibe on|off|beat; Head: head follow on|off");
    Serial.println("Routine: pomodoro start|stop; water on|off|now; day");
    Serial.println("Quote: quote");
    Serial.println("Pan: pan 0..180, look left, look right, look center");
    Serial.println("Info: time, weather, status card");
    Serial.println("Faces: happy, listen, speak, laugh, sleep, surprise, sad, curious, worried, idle");
    return;
  }

  if (command == "status") {
    Serial.print("WiFi: ");
    Serial.print(wifiConnected ? "connected" : "offline");
    Serial.print(" status=");
    Serial.print(WiFi.status());
    if (wifiConnected) {
      Serial.print(" IP=");
      Serial.print(WiFi.localIP());
      Serial.print(" RSSI=");
      Serial.print(WiFi.RSSI());
    }
    Serial.print(" mode=");
    Serial.print(personalityMode);
    Serial.print(" motors=");
    Serial.print(motorsEnabled ? "on" : "off");
    Serial.print(" checkins=");
    Serial.print(checkInsEnabled ? "on" : "off");
    Serial.print(" pan=");
    Serial.println(panAngle);
    return;
  }

  if (command == "llm test") {
    askLLM("Reply with exactly: LLM connection works.");
    return;
  }

  if (command == "checkins on") {
    checkInsEnabled = true;
    scheduleNextCheckIn();
    Serial.println("OK random check-ins enabled");
    return;
  }

  if (command == "checkins off") {
    checkInsEnabled = false;
    checkInWaitingForReply = false;
    Serial.println("OK random check-ins disabled");
    return;
  }

  if (command == "checkin now") {
    startCheckIn();
    return;
  }

  if (command == "vibe on") {
    vibeEnabled = true;
    vibeStep = 0;
    nextVibeAt = 0;
    Serial.println("OK vibe mode on; use this while music is playing");
    return;
  }

  if (command == "vibe off") {
    vibeEnabled = false;
    stopMotors();
    setState("idle");
    Serial.println("OK vibe mode off");
    return;
  }

  if (command == "vibe beat") {
    vibeEnabled = true;
    nextVibeAt = 0;
    Serial.println("OK vibe beat");
    return;
  }

  if (command == "head follow on") {
    headFollowEyes = true;
    Serial.println("OK head follows eye direction");
    return;
  }

  if (command == "head follow off") {
    headFollowEyes = false;
    movePan(PAN_CENTER);
    Serial.println("OK head follow off");
    return;
  }

  if (command == "pomodoro start") {
    startPomodoro();
    return;
  }

  if (command == "pomodoro stop") {
    stopPomodoro();
    return;
  }

  if (command == "water on") {
    waterReminderEnabled = true;
    nextWaterReminderAt = millis() + waterIntervalMs;
    Serial.println("OK water reminders on");
    return;
  }

  if (command == "water off") {
    waterReminderEnabled = false;
    Serial.println("OK water reminders off");
    return;
  }

  if (command == "water now") {
    waterReminders++;
    enqueueNotification("Water reminder: take a drink");
    nextWaterReminderAt = millis() + waterIntervalMs;
    return;
  }

  if (command == "day") {
    Serial.print("DAY uptime_min=");
    Serial.print((millis() - dayStartedAt) / 60000UL);
    Serial.print(" questions=");
    Serial.print(questionsToday);
    Serial.print(" checkins=");
    Serial.print(checkInsToday);
    Serial.print(" water=");
    Serial.print(waterReminders);
    Serial.print(" pomodoros=");
    Serial.print(pomodoroSessions);
    Serial.print(" last=");
    Serial.println(lastDayActivity);
    return;
  }

  if (command == "time") {
    showInfoCard(1, 5000);
    Serial.print("TIME: ");
    Serial.println(currentTimeText());
    return;
  }

  if (command == "weather") {
    showInfoCard(2, 6000);
    Serial.print("WEATHER: ");
    Serial.println(weatherSummary);
    return;
  }

  if (command == "status card") {
    showInfoCard(3, 6500);
    return;
  }

  if (command == "quote") {
    Serial.println("If it's man-made, I can make it.");
    enqueueNotification("If it's man-made, I can make it.");
    return;
  }

  if (command.startsWith("weather ")) {
    weatherSummary = command.substring(8);
    weatherSummary.trim();
    if (weatherSummary.length() > 42) weatherSummary = weatherSummary.substring(0, 42);
    recordActivity("Weather updated");
    Serial.print("WEATHER: ");
    Serial.println(weatherSummary);
    showInfoCard(2, 6000);
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

  if (command.startsWith("state ")) {
    String requestedState = command.substring(6);
    setState(requestedState);
    Serial.print("OK state ");
    Serial.println(requestedState);
    return;
  }

  if (command.startsWith("mode ")) {
    String requestedMode = command.substring(5);
    if (requestedMode == "calm" || requestedMode == "musical" || requestedMode == "engineer" || requestedMode == "quiet" || requestedMode == "spartan") {
      personalityMode = requestedMode;
      applyPersonalityFaceStyle();
      nextLedAt = 0;
      setPersonalityRgb();
      showIdentityReveal(requestedMode);
      drawFace();
      Serial.print("OK mode ");
      Serial.println(personalityMode);
    } else {
      Serial.println("ERR mode must be calm, musical, engineer, quiet, or spartan");
    }
    return;
  }

  if (command == "motors on") {
    motorsEnabled = true;
    Serial.println("OK motors enabled; use motors test with wheels lifted");
    return;
  }

  if (command == "motors off") {
    motorsEnabled = false;
    stopMotors();
    Serial.println("OK motors disabled");
    return;
  }

  if (command == "motors test") {
    if (!motorsEnabled) {
      Serial.println("ERR motors are off; type motors on first");
    } else {
      driveMotors(130, 130, 500);
      Serial.println("OK motor test: forward 0.5 seconds; lift wheels first");
    }
    return;
  }

  if (command == "drive forward") {
    driveMotors(motorSpeed, motorSpeed, 700);
    Serial.println("OK drive forward 0.7 seconds");
    return;
  }

  if (command == "drive backward") {
    driveMotors(-motorSpeed, -motorSpeed, 700);
    Serial.println("OK drive backward 0.7 seconds");
    return;
  }

  if (command == "turn left") {
    driveMotors(-motorSpeed, motorSpeed, 500);
    Serial.println("OK turn left 0.5 seconds");
    return;
  }

  if (command == "turn right") {
    driveMotors(motorSpeed, -motorSpeed, 500);
    Serial.println("OK turn right 0.5 seconds");
    return;
  }

  if (command == "dance" || command == "dance rocky") {
    startDance();
    Serial.println("OK dance rocky");
    return;
  }

  if (command == "stop" || command == "dance stop") {
    stopDance();
    vibeEnabled = false;
    setState("idle");
    Serial.println("OK stopped");
    return;
  }

  if (command.startsWith("notify ")) {
    enqueueNotification(command.substring(7));
    Serial.println("OK notification queued");
    return;
  }

  if (command == "read notifications") {
    if (pendingNotification.length() == 0) {
      Serial.println("OK no notifications");
    } else {
      setState("notification");
      Serial.print("NOTIFICATION: ");
      Serial.println(pendingNotification);
      pendingNotification = "";
    }
    return;
  }

  if (command.startsWith("ask ")) {
    askLLM(command.substring(4));
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
         face == "speak" || face == "thinking" || face == "sleep" || face == "surprise" ||
         face == "sad" || face == "curious" || face == "worried" || face == "notification" ||
         face == "left" || face == "right" || face == "laugh";
}

void setFace(String face) {
  blinking = false;
  mouthOpen = false;
  eyeJitter = 0;
  characterMotionStep = 0;
  nextCharacterMotionAt = millis() + 700;
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
  } else if (face == "thinking") {
    faceMode = FACE_THINKING;
    eyeMood = 1;
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
  } else if (face == "notification") {
    faceMode = FACE_NOTIFICATION;
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
  } else if (face == "laugh") {
    faceMode = FACE_LAUGH;
    eyeMood = 0;
    eyePosition = 16;
  }

  applyPersonalityFaceStyle();
  schedulePersonality();
  drawFace();
}

int personalityEyeMood() {
  if (personalityMode == "musical") return 3;
  if (personalityMode == "engineer") return 1;
  if (personalityMode == "quiet") return 4;
  if (personalityMode == "spartan") return 5;
  return 0;
}

void applyPersonalityFaceStyle() {
  if (faceMode == FACE_SLEEP) {
    eyeMood = 2;
  } else {
    eyeMood = personalityEyeMood();
  }
}

void schedulePersonality() {
  unsigned long now = millis();
  nextBlinkAt = now + random(3500, 7500);
  nextIdleGlanceAt = now + random(2500, 6000);
  nextMouthFrameAt = now + 180;
}

void scheduleNextCheckIn() {
  // A relaxed, non-predictable interval: approximately 3–8 minutes.
  nextCheckInAt = millis() + random(180000L, 480000L);
}

String buildCheckInPrompt() {
  if (personalityMode == "spartan") {
    return "Be a wise, highly driven Spartan desk companion. Check on the human directly, motivate disciplined progress, and offer one strong practical action. You may use the motto: If it's man-made, I can make it. Keep it under 100 characters.";
  }
  if (personalityMode == "engineer") {
    return "Proactively check on the human during a build session. Ask one practical question about their code or electronics progress and offer one small actionable suggestion. Keep it under 100 characters.";
  }
  if (personalityMode == "musical") {
    return "Proactively check on the human with a warm, playful, original little desk-buddy thought. Ask gently how they are doing, with a light musical rhythm but no imitation of any film voice. Keep it under 100 characters.";
  }
  if (personalityMode == "quiet") {
    return "Proactively check on the human with a very brief calm question, six words or fewer. Do not sound needy.";
  }
  return "Proactively check on the human in a calm, relaxing way. Ask whether they need a short break or help, without sounding needy. Keep it under 100 characters.";
}

void startCheckIn() {
  if (!checkInsEnabled) {
    Serial.println("CHECK-IN disabled; use checkins on first");
    return;
  }
  scheduleNextCheckIn();
  if (String(LLM_API_KEY).length() == 0) {
    Serial.println("CHECK-IN skipped: add LLM_API_KEY to private arduino_secrets.h");
    return;
  }
  if (WiFi.status() != WL_CONNECTED || !hasValidLocalIP()) {
    Serial.println("CHECK-IN skipped: Wi-Fi is not ready");
    return;
  }
  checkInWaitingForReply = true;
  checkInsToday++;
  recordActivity("Check-in started");
  checkInListeningUntil = millis() + 900;
  setState("listening");
  Serial.println("CHECK-IN: listening briefly before asking");
}

void movePan(int newAngle) {
  panAngle = constrain(newAngle, PAN_MIN, PAN_MAX);
  panServo.write(panAngle);
}

void setupMotors() {
  pinMode(LEFT_EN_PIN, OUTPUT);
  pinMode(LEFT_IN1_PIN, OUTPUT);
  pinMode(LEFT_IN2_PIN, OUTPUT);
  pinMode(RIGHT_EN_PIN, OUTPUT);
  pinMode(RIGHT_IN1_PIN, OUTPUT);
  pinMode(RIGHT_IN2_PIN, OUTPUT);
  stopMotors();
}

void setMotor(int left, int right) {
  if (!motorsEnabled) {
    left = 0;
    right = 0;
  }
  left = constrain(left, -255, 255);
  right = constrain(right, -255, 255);

  digitalWrite(LEFT_IN1_PIN, left > 0 ? HIGH : LOW);
  digitalWrite(LEFT_IN2_PIN, left < 0 ? HIGH : LOW);
  analogWrite(LEFT_EN_PIN, abs(left));

  digitalWrite(RIGHT_IN1_PIN, right > 0 ? HIGH : LOW);
  digitalWrite(RIGHT_IN2_PIN, right < 0 ? HIGH : LOW);
  analogWrite(RIGHT_EN_PIN, abs(right));
}

void stopMotors() {
  analogWrite(LEFT_EN_PIN, 0);
  analogWrite(RIGHT_EN_PIN, 0);
  digitalWrite(LEFT_IN1_PIN, LOW);
  digitalWrite(LEFT_IN2_PIN, LOW);
  digitalWrite(RIGHT_IN1_PIN, LOW);
  digitalWrite(RIGHT_IN2_PIN, LOW);
  motorMotionEndsAt = 0;
}

void driveMotors(int left, int right, unsigned long durationMs) {
  if (!motorsEnabled) {
    Serial.println("MOTORS OFF: type motors on first");
    stopMotors();
    return;
  }
  setMotor(left, right);
  motorMotionEndsAt = millis() + min(durationMs, 1500UL);
}

void updateMotorSafety() {
  if (motorMotionEndsAt != 0 && millis() >= motorMotionEndsAt) {
    stopMotors();
  }
}

void connectWiFi() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WIFI ERROR: WiFi module not detected");
    wifiConnected = false;
    nextWifiAttemptAt = millis() + 30000;
    drawFace();
    return;
  }

  wifiAttemptCount++;
  Serial.print("Connecting to Wi-Fi attempt ");
  Serial.print(wifiAttemptCount);
  Serial.print(" SSID=");
  Serial.println(wifiSsid);
  Serial.print("WiFi firmware: ");
  Serial.println(WiFi.firmwareVersion());
  Serial.println("Check SSID/password, 2.4 GHz network, WPA/WPA2, and signal if this fails.");
  unsigned long started = millis();
  wifiStatus = WiFi.begin(wifiSsid, wifiPass);
  while (WiFi.status() != WL_CONNECTED && millis() - started < 12000) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi associated; waiting for DHCP address...");
    unsigned long dhcpStarted = millis();
    while (!hasValidLocalIP() && millis() - dhcpStarted < 10000) {
      delay(250);
      Serial.print('D');
    }
    Serial.println();
  }

  if (WiFi.status() == WL_CONNECTED && hasValidLocalIP()) {
    wifiStatus = WL_CONNECTED;
    wifiConnected = true;
    server.begin();
    fallbackServer.begin();
    Serial.print("WIFI OK IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal RSSI: ");
    Serial.println(WiFi.RSSI());
    Serial.print("DASHBOARD: http://");
    Serial.println(WiFi.localIP());
    Serial.print("DASHBOARD FALLBACK: http://");
    Serial.print(WiFi.localIP());
    Serial.println(":8080");
    Serial.println("HTTP: /health, /api/status, /cmd/... and /input/...");
    drawFace();
  } else {
    wifiConnected = false;
    wifiStatus = WiFi.status();
    Serial.println("WIFI ERROR: connected radio but no usable DHCP IP; continuing with USB serial only");
    Serial.print("WiFi status code: ");
    Serial.println(wifiStatus);
    Serial.println("Common causes: wrong credentials, phone hotspot set to 5 GHz, captive portal, or weak signal.");
    nextWifiAttemptAt = millis() + 10000;
    drawFace();
  }
}

bool hasValidLocalIP() {
  IPAddress ip = WiFi.localIP();
  return ip[0] != 0 || ip[1] != 0 || ip[2] != 0 || ip[3] != 0;
}

void maintainWiFi() {
  int currentStatus = WiFi.status();
  if (currentStatus == WL_CONNECTED && hasValidLocalIP()) {
    if (!wifiConnected) {
      wifiConnected = true;
      wifiStatus = WL_CONNECTED;
      server.begin();
      fallbackServer.begin();
      Serial.print("WIFI RECONNECTED IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("DASHBOARD FALLBACK: http://");
      Serial.print(WiFi.localIP());
      Serial.println(":8080");
      drawFace();
    }
    return;
  }

  if (wifiConnected) {
    wifiConnected = false;
    Serial.println("WIFI LOST: USB serial and local face remain available");
    drawFace();
  }

  if (millis() >= nextWifiAttemptAt) {
    connectWiFi();
  }
}

void handleWiFiClient() {
  handleWiFiClientOn(server);
  handleWiFiClientOn(fallbackServer);
}

void handleWiFiClientOn(WiFiServer &activeServer) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client = activeServer.available();
  if (!client) return;

  client.setTimeout(1500);

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
    if (path == "/" || path == "/index.html") {
      sendDashboard(client);
      client.stop();
      return;
    } else if (path == "/health") {
      body = "OK IP=" + WiFi.localIP().toString() + " PAN=" + String(panAngle) + " MODE=" + personalityMode;
    } else if (path == "/api/status") {
      body = buildStatusJson();
      sendHttpResponse(client, "application/json", body);
      client.stop();
      return;
    } else if (path == "/last") {
      body = lastLLMReply.length() == 0 ? "{}" : lastLLMReply;
    } else if (path.startsWith("/cmd/")) {
      String command = urlDecode(path.substring(5));
      processCommand(command);
      body = "OK command=" + command;
    } else if (path.startsWith("/input/")) {
      String prompt = urlDecode(path.substring(7));
      askLLM(prompt);
      body = "OK input received";
    } else if (path.startsWith("/reply/")) {
      String reply = urlDecode(path.substring(7));
      if (reply.length() > 180) reply = reply.substring(0, 180);
      lastLLMReply = "{\"reply\":\"" + jsonEscape(reply) + "\"}";
      llmReplySequence++;
      setSpeakingText(reply);
      setState("speaking");
      recordActivity("External reply received");
      body = "OK external reply received";
    } else {
      body = "Use /health, /api/status, /cmd/..., /input/your%20text, or /reply/your%20text";
    }
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain; charset=utf-8");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println("Content-Length: " + String(body.length()));
  client.println();
  client.print(body);
  client.stop();
}

void sendHttpResponse(WiFiClient &client, const String &contentType, const String &body) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: " + contentType + "; charset=utf-8");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println("Content-Length: " + String(body.length()));
  client.println();
  client.print(body);
}

String buildStatusJson() {
  String body;
  body.reserve(620);
  String rgb = "blue";
  if (interactionState == STATE_ERROR) rgb = "red";
  else if (interactionState == STATE_THINKING) rgb = "cyan";
  else if (interactionState == STATE_LISTENING) rgb = "purple";
  else if (interactionState == STATE_NOTIFICATION) rgb = "amber";
  else if (interactionState == STATE_SPEAKING) rgb = "green";
  else if (interactionState == STATE_DANCE) rgb = "magenta";
  else if (personalityMode == "engineer") rgb = "cyan";
  else if (personalityMode == "spartan") rgb = "red-orange";
  else if (personalityMode == "musical") rgb = "violet";
  else if (personalityMode == "quiet") rgb = "white";

  body += "{\"wifi\":";
  body += wifiConnected ? "true" : "false";
  body += ",\"ip\":\"";
  body += WiFi.localIP().toString();
  body += "\",\"mode\":\"";
  body += jsonEscape(personalityMode);
  body += "\",\"motors\":";
  body += motorsEnabled ? "true" : "false";
  body += ",\"pan\":";
  body += String(panAngle);
  body += ",\"checkins\":";
  body += checkInsEnabled ? "true" : "false";
  body += ",\"vibe\":";
  body += vibeEnabled ? "true" : "false";
  body += ",\"head\":";
  body += headFollowEyes ? "true" : "false";
  body += ",\"info_card\":";
  body += infoCardActive ? "true" : "false";
  body += ",\"rgb\":\"";
  body += rgb;
  body += "\",\"water\":";
  body += waterReminderEnabled ? "true" : "false";
  body += ",\"pomodoro\":";
  body += pomodoroActive ? "true" : "false";
  body += ",\"pomodoros\":";
  body += String(pomodoroSessions);
  body += ",\"water_count\":";
  body += String(waterReminders);
  body += ",\"questions\":";
  body += String(questionsToday);
  body += ",\"checkin_count\":";
  body += String(checkInsToday);
  body += ",\"seq\":";
  body += String(llmReplySequence);
  body += ",\"reply\":";
  body += lastLLMReply.length() > 0 ? "true" : "false";
  body += ",\"text\":\"";
  body += jsonEscape(lastLLMReply.length() > 0 ? extractLLMReplyText(lastLLMReply) : "");
  body += "\",\"time\":\"";
  body += currentTimeText();
  body += "\",\"weather\":\"";
  body += jsonEscape(weatherSummary);
  body += "\"}";
  return body;
}

void sendDashboard(WiFiClient &client) {
  String html = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Rocky Desk Buddy</title><style>
body{font-family:system-ui,sans-serif;max-width:720px;margin:auto;padding:16px;background:#10131a;color:#f4f7fb}h1{margin:0 0 8px}.card{background:#1b2230;border:1px solid #354158;border-radius:14px;padding:14px;margin:12px 0}button{font-size:16px;border:0;border-radius:10px;padding:12px 14px;margin:5px;background:#3b82f6;color:white}button.warn{background:#e06c39}button.stop{background:#bd334b}button.mode{background:#7257b8}input{font-size:16px;padding:12px;border-radius:9px;border:1px solid #63708a;background:#0f141d;color:white;width:65%}#status{font-family:monospace;white-space:pre-wrap;color:#b5c6e6;line-height:1.45}.small{font-size:13px;color:#aab6c9}</style></head><body>
<h1>Rocky Desk Buddy</h1><div class="small">UNO R4 WiFi dashboard · current bridge architecture</div>
<div class="card"><b>Status</b><div id="status">Loading...</div><button onclick="refresh()">Refresh</button></div>
<div class="card"><b>Face and interaction</b><br><button onclick="cmd('state idle')">Idle</button><button onclick="cmd('state listening')">Listening</button><button onclick="cmd('state thinking')">Thinking</button><button onclick="cmd('state speaking')">Speak</button><button onclick="cmd('laugh')">Laugh</button><button onclick="cmd('state happy')">Happy</button><button onclick="cmd('surprise')">Surprise</button><button onclick="cmd('sad')">Sad</button><button onclick="cmd('curious')">Curious</button><button onclick="cmd('worried')">Worried</button><button onclick="cmd('state sleep')">Sleep</button><button onclick="cmd('state notification')">Notification</button><button class="stop" onclick="cmd('state error')">Error</button></div>
<div class="card"><b>Mouth and look</b><br><button onclick="cmd('mouth smile')">Smile mouth</button><button onclick="cmd('mouth open')">Open mouth</button><button onclick="cmd('mouth flat')">Flat mouth</button><br><button onclick="cmd('look left')">Look left</button><button onclick="cmd('look center')">Center</button><button onclick="cmd('look right')">Look right</button><button class="warn" onclick="cmd('dance rocky')">Long dance</button><button class="stop" onclick="cmd('stop')">STOP</button></div>
<div class="card"><b>Motors</b><div class="small">Motors start disabled. Lift wheels before testing.</div><button onclick="cmd('motors on')">Enable</button><button onclick="cmd('motors test')">Test</button><button onclick="cmd('drive forward')">Forward</button><button onclick="cmd('drive backward')">Backward</button><button onclick="cmd('turn left')">Left</button><button onclick="cmd('turn right')">Right</button><button onclick="cmd('motors off')">Disable</button></div>
<div class="card"><b>Pan position</b><br><button onclick="cmd('pan 0')">Pan 0°</button><button onclick="cmd('pan 90')">Center 90°</button><button onclick="cmd('pan 180')">Pan 180°</button></div>
<div class="card"><b>Personality / LLM</b><br><button class="mode" onclick="cmd('mode calm')">Calm</button><button class="mode" onclick="cmd('mode musical')">Musical</button><button class="mode" onclick="cmd('mode engineer')">Engineer</button><button class="mode" onclick="cmd('mode quiet')">Quiet</button><button class="mode" onclick="cmd('mode spartan')">Spartan</button><br><button onclick="cmd('llm test')">LLM test</button><button onclick="cmd('checkin now')">Check in now</button><button onclick="cmd('quote')">Motto</button><button onclick="toggleVoice()" id="voiceBtn">Browser voice: ON</button><br><input id="question" placeholder="Ask electronics or code question"><button onclick="ask()">Ask</button></div>
<div class="card"><b>Random check-ins</b><div class="small">Different approach for each personality; enabled by default when an API key is configured.</div><button onclick="cmd('checkins on')">Enable</button><button onclick="cmd('checkins off')">Disable</button></div>
<div class="card"><b>Music vibe</b><div class="small">Start this while music is playing. It adds a repeating beat-like pan/face dance; true audio beat sync will come with a microphone later.</div><button class="warn" onclick="cmd('vibe on')">Vibe ON</button><button onclick="cmd('vibe beat')">Beat</button><button onclick="cmd('vibe off')">Vibe OFF</button></div>
<div class="card"><b>Routine</b><br><button onclick="cmd('pomodoro start')">Pomodoro 25m</button><button onclick="cmd('pomodoro stop')">Stop Pomodoro</button><button onclick="cmd('water now')">Water now</button><button onclick="cmd('water on')">Water ON</button><button onclick="cmd('water off')">Water OFF</button><button onclick="cmd('day')">Day summary</button></div>
<div class="card"><b>Head follow</b><div class="small">When enabled, the servo follows the eye direction and idle glances. Manual pan still works when disabled.</div><button onclick="cmd('head follow on')">Follow eyes</button><button onclick="cmd('head follow off')">Center/manual</button></div>
<div class="card"><b>Time and weather</b><div class="small">Time comes from Wi-Fi. Cards appear on demand or after long idle periods.</div><button onclick="cmd('time')">Show time</button><button onclick="cmd('weather')">Show weather</button><button onclick="cmd('status card')">Show status</button><br><input id="place" placeholder="City, e.g. Hyderabad"><button onclick="weatherNow()">Get weather</button><div id="weatherResult" class="small"></div></div>
<div class="card"><b>Notifications</b><br><input id="note" placeholder="e.g. firmware build completed"><button onclick="notify()">Queue notification</button><button onclick="cmd('read notifications')">Read latest</button></div>
<div class="card"><b>Latest response</b><div id="replyText">No response yet.</div></div><div id="result" class="small"></div><script>
async function get(path){let r=await fetch(path);let t=await r.text();document.getElementById('result').textContent=t;refresh();}
function cmd(c){get('/cmd/'+encodeURIComponent(c))}function ask(){let v=document.getElementById('question').value;if(v)get('/input/'+encodeURIComponent(v))}function notify(){let v=document.getElementById('note').value;if(v)get('/cmd/'+encodeURIComponent('notify '+v))}
let lastSeq=0,speakEnabled=true;function toggleVoice(){speakEnabled=!speakEnabled;document.getElementById('voiceBtn').textContent='Browser voice: '+(speakEnabled?'ON':'off');if(!speakEnabled&&'speechSynthesis'in window)speechSynthesis.cancel()}
async function speakLatest(mode,seq){if(!speakEnabled||!('speechSynthesis'in window)||seq===lastSeq)return;let raw=await (await fetch('/last')).text(),text='';try{let body=raw.split('\r\n\r\n').pop(),d=JSON.parse(body);text=d.reply||((d.choices&&d.choices[0]&&d.choices[0].message&&d.choices[0].message.content)||'');if(typeof text==='string'&&text.trim().startsWith('{'))text=JSON.parse(text).reply||text;}catch(e){}if(!text)return;let u=new SpeechSynthesisUtterance(text);let style={calm:[.88,.96],musical:[1.0,1.12],engineer:[1.05,.90],quiet:[.76,.96],spartan:[1.02,.88]}[mode]||[.88,.96];u.rate=style[0];u.pitch=style[1];u.volume=.78;let words=mode==='engineer'||mode==='spartan'?['david','alex','male']:['zira','samantha','female'];let v=speechSynthesis.getVoices().find(x=>words.some(w=>(x.name+' '+x.lang).toLowerCase().includes(w)));if(v)u.voice=v;speechSynthesis.cancel();speechSynthesis.speak(u)}
async function weatherNow(){let place=document.getElementById('place').value.trim();if(!place)return;try{let g=await (await fetch('https://geocoding-api.open-meteo.com/v1/search?name='+encodeURIComponent(place)+'&count=1&language=en&format=json')).json();if(!g.results||!g.results.length)throw Error('City not found');let p=g.results[0];let f=await (await fetch('https://api.open-meteo.com/v1/forecast?latitude='+p.latitude+'&longitude='+p.longitude+'&current=temperature_2m,weather_code&timezone=auto')).json();let c=f.current;let text=p.name+': '+c.temperature_2m+'°C, code '+c.weather_code;document.getElementById('weatherResult').textContent=text;cmd('weather '+text);}catch(e){document.getElementById('weatherResult').textContent='Weather lookup failed: '+e.message;}}
async function refresh(){try{let r=await fetch('/api/status',{cache:'no-store'});let raw=await r.text();if(!r.ok)throw Error('HTTP '+r.status+' '+raw.slice(0,120));let s=JSON.parse(raw);document.getElementById('status').textContent='Wi-Fi: '+(s.wifi?'CONNECTED':'OFFLINE')+'\nIP: '+s.ip+'\nTime: '+s.time+'\nWeather: '+s.weather+'\nMode: '+s.mode+'\nRGB: '+s.rgb+'\nInfo card: '+(s.info_card?'ON':'face')+'\nMotors: '+(s.motors?'ENABLED':'off')+'\nVibe: '+(s.vibe?'ON':'off')+'\nHead follow: '+(s.head?'ON':'off')+'\nPomodoro: '+(s.pomodoro?'ON':'off')+'\nWater reminders: '+(s.water?'ON':'off')+' ('+s.water_count+')\nQuestions: '+s.questions+'  Check-ins: '+s.checkin_count+'  Pomodoros: '+s.pomodoros;document.getElementById('replyText').textContent=s.text||'No response yet.';if(s.seq!==lastSeq){await speakLatest(s.mode,s.seq);lastSeq=s.seq;}}catch(e){document.getElementById('status').textContent='Status API error: '+e.message;}}refresh();setInterval(refresh,3000);
</script></body></html>)HTML";
  sendHttpResponse(client, "text/html", html);
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

void setState(String state) {
  state.trim();
  state.toLowerCase();
  infoCardActive = false;

  if (state == "idle") {
    danceActive = false;
    checkInWaitingForReply = false;
    stopMotors();
    interactionState = STATE_IDLE;
    setFace("idle");
  } else if (state == "listening" || state == "listen") {
    interactionState = STATE_LISTENING;
    setFace("listen");
  } else if (state == "thinking") {
    interactionState = STATE_THINKING;
    setFace("thinking");
  } else if (state == "speaking" || state == "speak") {
    interactionState = STATE_SPEAKING;
    speakingEndsAt = millis() + 6000;
    setFace("speak");
  } else if (state == "happy") {
    interactionState = STATE_HAPPY;
    setFace("happy");
  } else if (state == "laugh") {
    interactionState = STATE_HAPPY;
    setFace("laugh");
  } else if (state == "look left") {
    movePan(PAN_MIN);
    interactionState = STATE_HAPPY;
    setFace("left");
  } else if (state == "look right") {
    movePan(PAN_MAX);
    interactionState = STATE_HAPPY;
    setFace("right");
  } else if (state == "look center") {
    movePan(PAN_CENTER);
    interactionState = STATE_IDLE;
    setFace("idle");
  } else if (state == "dance") {
    startDance();
  } else if (state == "notification") {
    interactionState = STATE_NOTIFICATION;
    setFace("notification");
    notificationEndsAt = millis() + 2500;
  } else if (state == "error") {
    danceActive = false;
    stopMotors();
    interactionState = STATE_ERROR;
    setFace("worried");
  } else if (state == "sleep") {
    danceActive = false;
    stopMotors();
    interactionState = STATE_SLEEP;
    setFace("sleep");
  } else {
    Serial.println("ERR unknown state");
  }
}

void startDance() {
  danceActive = true;
  interactionState = STATE_DANCE;
  danceStep = 0;
  nextDanceAt = 0;
  personalityOn = true;
}

void stopDance() {
  danceActive = false;
  danceStep = 0;
  movePan(PAN_CENTER);
  stopMotors();
}

void updateDance() {
  if (!danceActive) return;
  unsigned long now = millis();
  if (now < nextDanceAt) return;

  switch (danceStep) {
    case 0:
      movePan(PAN_CENTER);
      stopMotors();
      setFace("happy");
      nextDanceAt = now + 700;
      break;
    case 1:
      movePan(PAN_MIN + 8);
      if (motorsEnabled) driveMotors(-120, 120, 700);
      setFace("curious");
      nextDanceAt = now + 700;
      break;
    case 2:
      movePan(PAN_MAX - 8);
      if (motorsEnabled) driveMotors(120, -120, 700);
      setFace("curious");
      nextDanceAt = now + 700;
      break;
    case 3:
      movePan(PAN_MIN + 14);
      if (motorsEnabled) driveMotors(125, 125, 450);
      setFace("surprise");
      nextDanceAt = now + 450;
      break;
    case 4:
      movePan(PAN_MAX - 14);
      if (motorsEnabled) driveMotors(-115, -115, 450);
      setFace("laugh");
      nextDanceAt = now + 450;
      break;
    case 5:
      movePan(PAN_MIN + 5);
      if (motorsEnabled) driveMotors(105, 105, 550);
      setFace("happy");
      nextDanceAt = now + 550;
      break;
    case 6:
      movePan(PAN_MAX - 5);
      if (motorsEnabled) driveMotors(-105, -105, 550);
      setFace("laugh");
      nextDanceAt = now + 550;
      break;
    case 7:
      movePan(PAN_CENTER);
      stopMotors();
      setFace("happy");
      nextDanceAt = now + 1100;
      break;
    default:
      danceActive = false;
      interactionState = STATE_HAPPY;
      stopMotors();
      setFace("happy");
      return;
  }
  danceStep++;
}

void updateVibe() {
  if (!vibeEnabled || danceActive) return;
  unsigned long now = millis();
  if (now < nextVibeAt) return;

  switch (vibeStep % 6) {
    case 0:
      movePan(PAN_MIN + 8);
      setFace("happy");
      if (motorsEnabled) driveMotors(-90, 90, 450);
      nextVibeAt = now + 500;
      break;
    case 1:
      movePan(PAN_MAX - 8);
      setFace("laugh");
      if (motorsEnabled) driveMotors(90, -90, 450);
      nextVibeAt = now + 500;
      break;
    case 2:
      movePan(PAN_CENTER);
      setFace("happy");
      if (motorsEnabled) driveMotors(110, 110, 350);
      nextVibeAt = now + 400;
      break;
    case 3:
      movePan(PAN_MIN + 12);
      setFace("curious");
      if (motorsEnabled) driveMotors(-85, 85, 400);
      nextVibeAt = now + 450;
      break;
    case 4:
      movePan(PAN_MAX - 12);
      setFace("surprise");
      if (motorsEnabled) driveMotors(85, -85, 400);
      nextVibeAt = now + 450;
      break;
    default:
      movePan(PAN_CENTER);
      setFace("happy");
      stopMotors();
      nextVibeAt = now + 650;
      break;
  }
  vibeStep++;
}

void updateClock() {
  if (!wifiConnected) return;
  unsigned long now = millis();
  if (now < nextClockSyncAt) return;
  unsigned long networkEpoch = WiFi.getTime();
  if (networkEpoch > 0) {
    clockEpochBase = networkEpoch + TIMEZONE_OFFSET_SECONDS;
    clockMillisBase = now;
    clockReady = true;
    nextClockSyncAt = now + 60000UL;
    drawFace();
  } else {
    nextClockSyncAt = now + 10000UL;
  }
}

String currentTimeText() {
  if (!clockReady) return "--:--";
  time_t current = static_cast<time_t>(clockEpochBase + ((millis() - clockMillisBase) / 1000UL));
  struct tm *parts = gmtime(&current);
  if (!parts) return "--:--";
  char output[6];
  snprintf(output, sizeof(output), "%02d:%02d", parts->tm_hour, parts->tm_min);
  return String(output);
}

void drawInfoCard(uint8_t mode) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(2, 2);
  if (mode == 1) {
    display.println("TIME");
    display.setTextSize(2);
    display.setCursor(18, 22);
    display.println(currentTimeText());
    display.setTextSize(1);
    display.setCursor(2, 54);
    display.println("Wi-Fi synced");
  } else if (mode == 2) {
    display.println("WEATHER");
    display.setCursor(2, 20);
    String text = weatherSummary;
    if (text.length() > 20) text = text.substring(0, 20);
    display.println(text);
    display.setCursor(2, 38);
    display.println("Ask dashboard to refresh");
  } else if (mode == 4) {
    display.println("RESPONSE");
    String text = extractLLMReplyText(lastLLMReply);
    if (text.length() > 19) text = text.substring(0, 19);
    display.setCursor(2, 20);
    display.println(text);
    display.setCursor(2, 42);
    display.println("Speaking...");
  } else {
    display.println("ROCKY STATUS");
    display.setCursor(2, 17);
    display.print("Mode: ");
    display.println(personalityMode);
    display.setCursor(2, 30);
    display.print("Wi-Fi: ");
    display.println(wifiConnected ? "online" : "offline");
    display.setCursor(2, 43);
    display.print("Pan: ");
    display.print(panAngle);
    display.print("  Q:");
    display.println(questionsToday);
  }
}

void showInfoCard(uint8_t mode, unsigned long durationMs) {
  infoCardActive = true;
  infoCardMode = mode;
  infoCardEndsAt = millis() + durationMs;
  nextIdleInfoAt = millis() + 45000UL;
  drawFace();
}

void updateInfoCard() {
  unsigned long now = millis();
  if (infoCardActive) {
    if (now >= infoCardEndsAt) {
      infoCardActive = false;
      nextIdleInfoAt = now + random(45000UL, 90000UL);
      drawFace();
    }
    return;
  }
  if (interactionState == STATE_IDLE && !danceActive && !vibeEnabled && now >= nextIdleInfoAt) {
    showInfoCard(static_cast<uint8_t>(random(1, 4)), random(4500UL, 7000UL));
  }
}

void updateStatusLED() {
  unsigned long now = millis();
  if (now < nextLedAt) return;

  if (interactionState == STATE_ERROR) {
    ledState = !ledState;
    setRgb(ledState ? 255 : 0, 0, 0);
    nextLedAt = now + 180;
    return;
  }
  if (interactionState == STATE_THINKING) {
    ledState = !ledState;
    setRgb(0, ledState ? 180 : 0, ledState ? 255 : 0);
    nextLedAt = now + 350;
    return;
  }
  if (interactionState == STATE_NOTIFICATION || pomodoroBreak) {
    ledState = !ledState;
    setRgb(ledState ? 255 : 0, ledState ? 90 : 0, 0);
    nextLedAt = now + 700;
    return;
  }
  if (interactionState == STATE_LISTENING) {
    setRgb(180, 0, 255);
    nextLedAt = now + 1000;
    return;
  }
  if (interactionState == STATE_SPEAKING || interactionState == STATE_HAPPY || vibeEnabled) {
    setRgb(0, 255, 40);
    nextLedAt = now + 1000;
    return;
  }
  if (interactionState == STATE_DANCE) {
    setRgb(255, 0, 180);
    nextLedAt = now + 1000;
    return;
  }
  if (interactionState == STATE_SLEEP) {
    setRgb(0, 0, 40);
    nextLedAt = now + 1000;
    return;
  }
  setPersonalityRgb();
  nextLedAt = now + 1000;
}

void setRgb(uint8_t r, uint8_t g, uint8_t b) {
  bool redOn = r > 0;
  bool greenOn = g > 0;
  bool blueOn = b > 0;
  if (RGB_COMMON_ANODE) {
    digitalWrite(RGB_R_PIN, redOn ? LOW : HIGH);
    digitalWrite(RGB_G_PIN, greenOn ? LOW : HIGH);
    digitalWrite(RGB_B_PIN, blueOn ? LOW : HIGH);
  } else {
    digitalWrite(RGB_R_PIN, redOn ? HIGH : LOW);
    digitalWrite(RGB_G_PIN, greenOn ? HIGH : LOW);
    digitalWrite(RGB_B_PIN, blueOn ? HIGH : LOW);
  }
}

void setPersonalityRgb() {
  if (personalityMode == "engineer") {
    setRgb(0, 255, 255);       // cyan: discovery and analysis
  } else if (personalityMode == "spartan") {
    setRgb(255, 40, 0);        // red-orange: courage and drive
  } else if (personalityMode == "musical") {
    setRgb(180, 0, 255);       // violet: rhythm and imagination
  } else if (personalityMode == "quiet") {
    setRgb(255, 255, 255);     // soft white: quiet presence
  } else {
    setRgb(0, 80, 180);        // blue: calm default
  }
}

void startPomodoro() {
  pomodoroActive = true;
  pomodoroBreak = false;
  pomodoroEndsAt = millis() + 25UL * 60UL * 1000UL;
  recordActivity("Pomodoro started");
  enqueueNotification("Pomodoro started: 25 minutes");
}

void stopPomodoro() {
  pomodoroActive = false;
  pomodoroBreak = false;
  pomodoroEndsAt = 0;
  recordActivity("Pomodoro stopped");
  Serial.println("OK pomodoro stopped");
}

void updateReminders() {
  unsigned long now = millis();
  if (waterReminderEnabled && nextWaterReminderAt > 0 && now >= nextWaterReminderAt) {
    waterReminders++;
    enqueueNotification("Water reminder: take a drink");
    nextWaterReminderAt = now + waterIntervalMs;
    recordActivity("Water reminder");
  }

  if (pomodoroActive && pomodoroEndsAt > 0 && now >= pomodoroEndsAt) {
    if (!pomodoroBreak) {
      pomodoroSessions++;
      pomodoroBreak = true;
      pomodoroEndsAt = now + 5UL * 60UL * 1000UL;
      enqueueNotification("Pomodoro complete: take a 5 minute break");
      recordActivity("Pomodoro complete");
    } else {
      pomodoroBreak = false;
      pomodoroEndsAt = now + 25UL * 60UL * 1000UL;
      enqueueNotification("Break complete: ready for another Pomodoro");
      recordActivity("Break complete");
    }
  }
}

void syncHeadToEyes() {
  if (!headFollowEyes || danceActive || vibeEnabled) return;
  if (faceMode == FACE_LEFT || eyePosition == 0) {
    movePan(PAN_MIN);
  } else if (faceMode == FACE_RIGHT || eyePosition == 32) {
    movePan(PAN_MAX);
  } else {
    movePan(PAN_CENTER);
  }
}

void recordActivity(String activity) {
  lastDayActivity = activity;
}

void enqueueNotification(String message) {
  message.trim();
  if (message.length() == 0) return;
  if (message.length() > 75) message = message.substring(0, 75);
  pendingNotification = message;
  String activity = "Notification: ";
  activity += message;
  recordActivity(activity);
  if (interactionState == STATE_IDLE || interactionState == STATE_HAPPY) {
    setState("notification");
    Serial.print("NOTIFY: ");
    Serial.println(message);
  }
}

String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\r", "");
  value.replace("\n", " ");
  return value;
}

String findLLMAction(String response) {
  String marker = "\\\"action\\\":\\\"";
  int start = response.indexOf(marker);
  if (start < 0) {
    marker = "\"action\":\"";
    start = response.indexOf(marker);
  }
  if (start < 0) return "speaking";
  start += marker.length();
  int end = response.indexOf("\\\"", start);
  if (end < 0) end = response.indexOf("\"", start);
  if (end <= start) return "speaking";
  String action = response.substring(start, end);
  action.toLowerCase();

  if (action == "dance rocky" || action == "happy" || action == "listening" ||
      action == "thinking" || action == "speaking" || action == "sleep" ||
      action == "surprise" || action == "laugh" || action == "error" || action == "notification" ||
      action == "look left" || action == "look right" || action == "look center") {
    return action;
  }
  return "speaking";
}

String extractLLMReplyText(String response) {
  String marker = "\\\"reply\\\":\\\"";
  int start = response.indexOf(marker);
  if (start < 0) {
    marker = "\"reply\":\"";
    start = response.indexOf(marker);
  }
  if (start < 0) return "...";
  start += marker.length();
  int end = response.indexOf("\\\"", start);
  if (end < 0) end = response.indexOf("\"", start);
  if (end <= start) return "...";
  String reply = response.substring(start, end);
  reply.replace("\\n", " ");
  reply.replace("\\\"", "\"");
  return reply;
}

void askLLM(String prompt) {
  questionsToday++;
  recordActivity("Question sent");
  if (String(LLM_API_KEY).length() == 0) {
    Serial.println("LLM disabled: add LLM_API_KEY to private arduino_secrets.h");
    setState("error");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("LLM unavailable: Wi-Fi is not connected");
    setState("error");
    return;
  }

  setState("thinking");
  prompt.trim();
  if (prompt.length() > 220) prompt = prompt.substring(0, 220);

  String systemPrompt = "You are Rocky-Wall-E, an original desk robot. Current mode is " + personalityMode + ". Return JSON only with exactly action and reply. Allowed action values: idle, happy, listening, thinking, speaking, sleep, surprise, laugh, error, look left, look right, look center, dance rocky, notification. Reply must be one short sentence under 120 characters. In engineer mode, be a curious Einstein-inspired lab thinker: precise, inventive, humble, and safety-first; mention voltage, current, logic level, grounding, or safety when relevant, but do not imitate Einstein's voice or claim to be him. In musical mode, be warmly curious and slightly playful, but never imitate a film voice. In spartan mode, speak like a wise original Spartan mentor: disciplined, direct, courageous, highly motivated, and practical; the motto is If it's man-made, I can make it. Never invent GPIO commands.";
  String body = "{\"model\":\"" + String(LLM_MODEL) + "\",\"messages\":[{\"role\":\"system\",\"content\":\"" + jsonEscape(systemPrompt) + "\"},{\"role\":\"user\",\"content\":\"" + jsonEscape(prompt) + "\"}],\"max_tokens\":80,\"temperature\":0.2}";

  Serial.println("LLM: connecting");
  if (!llmClient.connect(LLM_HOST, 443)) {
    Serial.println("LLM ERROR: HTTPS connection failed");
    setState("error");
    return;
  }
  llmClient.println("POST " + String(LLM_PATH) + " HTTP/1.1");
  llmClient.println("Host: " + String(LLM_HOST));
  llmClient.println("Authorization: Bearer " + String(LLM_API_KEY));
  llmClient.println("Content-Type: application/json");
  llmClient.println("Connection: close");
  llmClient.println("Content-Length: " + String(body.length()));
  llmClient.println();
  llmClient.println(body);

  String response;
  unsigned long started = millis();
  while (millis() - started < 9000) {
    while (llmClient.available() && response.length() < 3500) {
      response += static_cast<char>(llmClient.read());
    }
    if (!llmClient.connected() && !llmClient.available()) break;
    delay(10);
  }
  llmClient.stop();

  if (response.indexOf("HTTP/1.1 200") < 0 && response.indexOf("HTTP/1.0 200") < 0) {
    Serial.println("LLM ERROR: server did not return HTTP 200");
    Serial.println(response.substring(0, min(180, response.length())));
    if (response.indexOf("401 Unauthorized") >= 0 || response.indexOf("HTTP/1.1 401") >= 0) {
      Serial.println("AUTH HINT: this direct path expects an OpenAI-compatible key for api.openai.com.");
      Serial.println("AUTH HINT: a Manus key uses x-manus-api-key at api.manus.ai through integration_relay/relay_server.py.");
    }
    setState("error");
    return;
  }

  String action = findLLMAction(response);
  lastLLMReply = response;
  llmReplySequence++;
  String replyText = extractLLMReplyText(response);
  setSpeakingText(replyText);
  Serial.print("LLM action: ");
  Serial.println(action);
  Serial.print("LLM reply: ");
  Serial.println(replyText);
  if (action == "dance rocky") {
    startDance();
  } else if (action == "laugh") {
    setState("laugh");
  } else if (action == "look left" || action == "look right" || action == "look center") {
    setState(action);
  } else {
    setState("speaking");
  }
}

void updatePersonality() {
  if (!personalityOn) return;

  unsigned long now = millis();

  if (interactionState == STATE_NOTIFICATION && notificationEndsAt > 0 && now >= notificationEndsAt) {
    notificationEndsAt = 0;
    setState("idle");
    return;
  }

  if (interactionState == STATE_SPEAKING && speakingEndsAt > 0 && now >= speakingEndsAt) {
    speakingEndsAt = 0;
    setState("idle");
    return;
  }

  if (checkInWaitingForReply && now >= checkInListeningUntil) {
    checkInWaitingForReply = false;
    askLLM(buildCheckInPrompt());
    return;
  }

  if (checkInsEnabled && nextCheckInAt > 0 && now >= nextCheckInAt &&
      interactionState == STATE_IDLE && !danceActive && !checkInWaitingForReply) {
    startCheckIn();
    return;
  }

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

  // Animate the mouth gently while the robot is speaking or laughing.
  if ((faceMode == FACE_SPEAK || faceMode == FACE_LAUGH) && now >= nextMouthFrameAt) {
    mouthOpen = !mouthOpen;
    unsigned long mouthCadence = 190;
    if (personalityMode == "musical") mouthCadence = random(110, 180);
    else if (personalityMode == "engineer") mouthCadence = random(150, 230);
    else if (personalityMode == "quiet") mouthCadence = random(250, 380);
    else if (personalityMode == "spartan") mouthCadence = random(180, 290);
    else mouthCadence = random(130, 260);
    nextMouthFrameAt = now + mouthCadence;
    drawFace();
    return;
  }

  if (interactionState == STATE_THINKING && now >= nextIdleGlanceAt) {
    eyePosition = random(0, 2) == 0 ? 0 : 32;
    nextIdleGlanceAt = now + random(500, 1100);
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

void updateCharacterAnimation() {
  if (blinking || infoCardActive || danceActive || vibeEnabled) return;
  unsigned long now = millis();
  if (now < nextCharacterMotionAt) return;

  if (personalityMode == "musical") {
    // A gentle alternating bounce and glance, never a blocking delay.
    characterMotionStep = (characterMotionStep + 1) % 4;
    eyeJitter = characterMotionStep == 1 ? 2 : (characterMotionStep == 3 ? -2 : 0);
    nextCharacterMotionAt = now + 420;
  } else if (personalityMode == "engineer") {
    // Focused left-to-right scan, like inspecting a circuit or waveform.
    characterMotionStep = (characterMotionStep + 1) % 5;
    eyePosition = characterMotionStep == 0 ? 0 : characterMotionStep == 4 ? 32 : 16;
    nextCharacterMotionAt = now + 650;
  } else if (personalityMode == "spartan") {
    // Deliberate, restrained focus: mostly center with occasional decisive glance.
    characterMotionStep = (characterMotionStep + 1) % 6;
    eyePosition = characterMotionStep == 2 ? 0 : (characterMotionStep == 4 ? 32 : 16);
    nextCharacterMotionAt = now + 900;
  } else if (personalityMode == "quiet") {
    // Soft breathing-like eye jitter.
    characterMotionStep = (characterMotionStep + 1) % 3;
    eyeJitter = characterMotionStep == 1 ? 1 : 0;
    nextCharacterMotionAt = now + 1100;
  } else {
    // Calm mode: rare, small movement.
    eyeJitter = 0;
    nextCharacterMotionAt = now + 1800;
  }
  drawFace();
}

void drawFace() {
  if (identityRevealActive) {
    drawIdentityCard();
    display.display();
    return;
  }
  if (infoCardActive) {
    drawInfoCard(infoCardMode);
    display.display();
    return;
  }
  display.clearDisplay();
  drawWifiIcon();
  drawPersonalityIcon();

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
  drawPersonalityOverlay();
  display.display();
}

void drawWifiIcon() {
  // Small status icon in the top-right corner: filled center plus arcs when
  // connected; a crossed mark when Wi-Fi is unavailable.
  if (wifiConnected) {
    display.fillCircle(121, 6, 1, SSD1306_WHITE);
    display.drawLine(117, 5, 121, 2, SSD1306_WHITE);
    display.drawLine(121, 2, 125, 5, SSD1306_WHITE);
    display.drawLine(115, 3, 121, 0, SSD1306_WHITE);
    display.drawLine(121, 0, 127, 3, SSD1306_WHITE);
  } else {
    display.drawLine(117, 1, 126, 7, SSD1306_WHITE);
    display.drawLine(126, 1, 117, 7, SSD1306_WHITE);
  }
}

void drawPersonalityIcon() {
  // Header icon area is 16x8 pixels at the top-left, above the eyes.
  if (personalityMode == "spartan") {
    // Minimal helmet and crest silhouette.
    display.drawLine(2, 6, 14, 6, SSD1306_WHITE);
    display.drawLine(3, 6, 4, 2, SSD1306_WHITE);
    display.drawLine(4, 2, 8, 1, SSD1306_WHITE);
    display.drawLine(8, 1, 12, 2, SSD1306_WHITE);
    display.drawLine(12, 2, 13, 6, SSD1306_WHITE);
    display.drawLine(8, 0, 8, 3, SSD1306_WHITE);
    display.drawLine(6, 1, 8, 0, SSD1306_WHITE);
    display.drawLine(8, 0, 10, 1, SSD1306_WHITE);
    display.drawLine(5, 4, 11, 4, SSD1306_WHITE);
  } else if (personalityMode == "engineer") {
    // Small original lab-thinker icon: hair spikes, round glasses, and nose.
    display.drawPixel(3, 1, SSD1306_WHITE);
    display.drawLine(4, 2, 6, 0, SSD1306_WHITE);
    display.drawLine(6, 0, 8, 2, SSD1306_WHITE);
    display.drawLine(8, 2, 10, 0, SSD1306_WHITE);
    display.drawLine(10, 0, 13, 2, SSD1306_WHITE);
    display.drawCircle(6, 4, 2, SSD1306_WHITE);
    display.drawCircle(11, 4, 2, SSD1306_WHITE);
    display.drawLine(8, 4, 9, 4, SSD1306_WHITE);
    display.drawLine(13, 4, 15, 3, SSD1306_WHITE);
    display.drawPixel(8, 7, SSD1306_WHITE);
  }
}

void showIdentityReveal(String mode) {
  identityRevealActive = true;
  if (mode == "engineer") identityRevealMode = 1;
  else if (mode == "spartan") identityRevealMode = 2;
  else if (mode == "musical") identityRevealMode = 3;
  else if (mode == "quiet") identityRevealMode = 4;
  else identityRevealMode = 5;
  identityRevealEndsAt = millis() + 2400UL;
}

void updateIdentityReveal() {
  if (identityRevealActive && millis() >= identityRevealEndsAt) {
    identityRevealActive = false;
    drawFace();
  }
}

void drawIdentityCard() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  if (identityRevealMode == 1) {
    // Stylized Einstein-inspired portrait: unmistakable hair, glasses,
    // moustache, and bow tie, rendered as original pixel art.
    display.setCursor(43, 0);
    display.println("EINSTEIN");
    display.fillTriangle(45, 15, 51, 6, 57, 14, SSD1306_WHITE);
    display.fillTriangle(53, 12, 62, 3, 68, 14, SSD1306_WHITE);
    display.fillTriangle(64, 14, 73, 4, 80, 15, SSD1306_WHITE);
    display.fillTriangle(76, 14, 84, 7, 89, 17, SSD1306_WHITE);
    display.fillCircle(64, 29, 18, SSD1306_WHITE);
    display.drawCircle(55, 27, 7, SSD1306_BLACK);
    display.drawCircle(73, 27, 7, SSD1306_BLACK);
    display.drawLine(62, 27, 66, 27, SSD1306_BLACK);
    display.drawLine(64, 29, 62, 34, SSD1306_BLACK);
    display.drawLine(62, 34, 67, 34, SSD1306_BLACK);
    display.drawLine(55, 36, 60, 33, SSD1306_BLACK);
    display.drawLine(60, 33, 64, 37, SSD1306_BLACK);
    display.drawLine(64, 37, 68, 33, SSD1306_BLACK);
    display.drawLine(68, 33, 73, 36, SSD1306_BLACK);
    display.drawLine(53, 45, 64, 57, SSD1306_WHITE);
    display.drawLine(75, 45, 64, 57, SSD1306_WHITE);
    display.fillTriangle(64, 48, 58, 55, 64, 61, SSD1306_WHITE);
    display.fillTriangle(64, 48, 70, 55, 64, 61, SSD1306_WHITE);
    display.setCursor(37, 57);
    display.println("CURIOUS MIND");
  } else if (identityRevealMode == 2) {
    // Stylized 300-inspired Spartan soldier: crest, helmet, face guard,
    // eye slit, and shield-like lower silhouette.
    display.setCursor(45, 0);
    display.println("SPARTAN");
    display.fillTriangle(64, 3, 51, 15, 77, 15, SSD1306_WHITE);
    display.fillRect(60, 3, 8, 7, SSD1306_WHITE);
    display.fillRoundRect(34, 13, 60, 34, 10, SSD1306_WHITE);
    display.fillRect(42, 26, 44, 7, SSD1306_BLACK);
    display.fillRect(48, 28, 32, 3, SSD1306_WHITE);
    display.fillTriangle(35, 28, 48, 31, 44, 51, SSD1306_WHITE);
    display.fillTriangle(93, 28, 80, 31, 84, 51, SSD1306_WHITE);
    display.drawLine(64, 16, 64, 23, SSD1306_BLACK);
    display.drawLine(47, 51, 81, 51, SSD1306_WHITE);
    display.drawLine(52, 55, 76, 55, SSD1306_WHITE);
    display.drawLine(58, 59, 70, 59, SSD1306_WHITE);
    display.setCursor(40, 57);
    display.println("DISCIPLINE");
  } else {
    display.setCursor(46, 0);
    if (identityRevealMode == 3) display.println("MUSICAL");
    else if (identityRevealMode == 4) display.println("QUIET");
    else display.println("CALM");
    display.drawRoundRect(24, 17, 32, 25, 8, SSD1306_WHITE);
    display.drawRoundRect(72, 17, 32, 25, 8, SSD1306_WHITE);
    display.drawLine(56, 29, 72, 29, SSD1306_WHITE);
    display.drawLine(48, 51, 80, 51, SSD1306_WHITE);
    display.setCursor(42, 57);
    display.println("DESK BUDDY");
  }
}

void setSpeakingText(String text) {
  text.trim();
  if (text.length() == 0) text = "...";
  if (text.length() > 48) text = text.substring(0, 48);
  speakingText = text;
}

void drawEyebrows() {
  switch (faceMode) {
    case FACE_IDLE:
      if (personalityMode == "spartan") {
        display.drawLine(18, 3, 37, 0, SSD1306_WHITE);
        display.drawLine(91, 0, 110, 3, SSD1306_WHITE);
      } else if (personalityMode == "engineer") {
        display.drawLine(20, 2, 36, 2, SSD1306_WHITE);
        display.drawLine(92, 2, 108, 2, SSD1306_WHITE);
      } else if (personalityMode == "musical") {
        display.drawLine(20, 3, 27, 0, SSD1306_WHITE);
        display.drawLine(27, 0, 36, 3, SSD1306_WHITE);
        display.drawLine(92, 3, 101, 0, SSD1306_WHITE);
        display.drawLine(101, 0, 108, 3, SSD1306_WHITE);
      }
      break;
    case FACE_HAPPY:
    case FACE_LAUGH:
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
    case FACE_THINKING:
      display.drawLine(20, 2, 36, 1, SSD1306_WHITE);
      display.drawLine(92, 1, 108, 2, SSD1306_WHITE);
      break;
    case FACE_NOTIFICATION:
      display.drawLine(20, 1, 36, 3, SSD1306_WHITE);
      display.drawLine(92, 3, 108, 1, SSD1306_WHITE);
      break;
    default:
      break;
  }
}

void drawCheeks() {
  if (faceMode == FACE_HAPPY || faceMode == FACE_LAUGH || faceMode == FACE_LISTEN || faceMode == FACE_NOTIFICATION || personalityMode == "musical") {
    // Small dotted cheeks, kept inside the 128x64 display area.
    display.drawPixel(10, 48, SSD1306_WHITE);
    display.drawPixel(13, 49, SSD1306_WHITE);
    display.drawPixel(114, 48, SSD1306_WHITE);
    display.drawPixel(117, 49, SSD1306_WHITE);
  }
}

void drawPersonalityOverlay() {
  if (personalityMode == "engineer") {
    // Small measurement ticks and playful lab-thinker hair cues: an original
    // Einstein-inspired engineer identity, not a portrait or likeness.
    display.drawLine(2, 14, 6, 14, SSD1306_WHITE);
    display.drawLine(2, 14, 2, 18, SSD1306_WHITE);
    display.drawLine(121, 48, 125, 48, SSD1306_WHITE);
    display.drawLine(8, 1, 13, 4, SSD1306_WHITE);
    display.drawLine(13, 4, 18, 0, SSD1306_WHITE);
    display.drawLine(110, 0, 115, 4, SSD1306_WHITE);
    display.drawLine(115, 4, 120, 1, SSD1306_WHITE);
    if (faceMode == FACE_IDLE || faceMode == FACE_THINKING || faceMode == FACE_SPEAK) {
      display.drawLine(55, 55, 60, 57, SSD1306_WHITE);
      display.drawLine(60, 57, 64, 55, SSD1306_WHITE);
      display.drawLine(64, 55, 68, 57, SSD1306_WHITE);
      display.drawLine(68, 57, 73, 55, SSD1306_WHITE);
    }
  } else if (personalityMode == "spartan") {
    // A restrained crest-like mark and strong lower accent.
    display.drawLine(60, 1, 64, 3, SSD1306_WHITE);
    display.drawLine(64, 3, 68, 1, SSD1306_WHITE);
    display.drawLine(58, 59, 70, 59, SSD1306_WHITE);
  } else if (personalityMode == "musical") {
    // Two tiny rhythm marks, kept away from the face.
    display.drawPixel(3, 54, SSD1306_WHITE);
    display.drawLine(5, 52, 5, 56, SSD1306_WHITE);
    display.drawPixel(123, 54, SSD1306_WHITE);
    display.drawLine(121, 52, 121, 56, SSD1306_WHITE);
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
    case FACE_IDLE:
      if (personalityMode == "musical") {
        display.drawLine(52, 50, 57, 53, SSD1306_WHITE);
        display.drawLine(57, 53, 64, 54, SSD1306_WHITE);
        display.drawLine(64, 54, 71, 53, SSD1306_WHITE);
        display.drawLine(71, 53, 76, 50, SSD1306_WHITE);
      } else if (personalityMode == "spartan") {
        display.drawLine(53, 51, 75, 51, SSD1306_WHITE);
        display.drawLine(57, 54, 71, 54, SSD1306_WHITE);
      } else {
        display.drawLine(58, 51, 70, 51, SSD1306_WHITE);
      }
      break;
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
    case FACE_LAUGH:
      if (mouthOpen) {
        display.fillRoundRect(50, 46, 28, 13, 5, SSD1306_WHITE);
        display.drawLine(55, 51, 73, 51, SSD1306_BLACK);
      } else {
        display.drawLine(50, 49, 55, 54, SSD1306_WHITE);
        display.drawLine(55, 54, 64, 57, SSD1306_WHITE);
        display.drawLine(64, 57, 73, 54, SSD1306_WHITE);
        display.drawLine(73, 54, 78, 49, SSD1306_WHITE);
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
    case FACE_THINKING:
      display.drawLine(57, 51, 71, 51, SSD1306_WHITE);
      display.drawPixel(75, 51, SSD1306_WHITE);
      break;
    case FACE_NOTIFICATION:
      display.drawCircle(64, 51, 4, SSD1306_WHITE);
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
