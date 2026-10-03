#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <ESP_I2S.h>
#include <time.h>
#include <esp_camera.h>
#include "../../boards/xiao-esp32s3-sense/pinmap.h"
#include "device_config.h"
#include "ota_service.h"
#include "../../modules/core/face_state.h"
#include "../../modules/core/personality.h"

/*
  Rocky XIAO Desk Buddy - modular Stage 2 bring-up

  This firmware is intentionally a hardware bring-up base, not the final voice
  firmware. It tests one function at a time:
    OLED -> servo -> DRV8833/N90 motors -> Wi-Fi/dashboard.

  Hardware-specific choices live in boards/xiao-esp32s3-sense/pinmap.h. No Wi-Fi password or API key
  is compiled into this sketch; Wi-Fi is provisioned through the setup AP.

  DRV8833 control model:
    AIN1/AIN2 control the left motor; BIN1/BIN2 control the right motor.
    PWM is applied directly to the selected input and the opposite input is LOW.
    nSLEEP/SLP must be tied to XIAO 3V3. nFAULT is optional and not used here.
*/

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#include "../../modules/faces/isabella_character.h"
#include "../../modules/faces/spartan_character.h"
#include "../../modules/faces/rocky_body_language.h"
namespace RockyBodyLanguage {
Adafruit_SSD1306 &display = ::display;
}
WebServer server(80);
Preferences preferences;
Servo headServo;
I2SClass microphone;
I2SClass audioOutput;
OtaService otaService;

// ------------------------------ State ------------------------------

FaceMode faceMode = FACE_IDLE;
Personality personality = PERSONALITY_CALM;
int panAngle = 90;
int motorSpeed = ROCKY_DEFAULT_MOTOR_SPEED;
String lastEvent = "Booting";
String lastCommand = "none";
String savedSsid;
String savedPassword;

bool provisioningAP = false;
unsigned long lastWiFiAttempt = 0;
unsigned long lastMotorCommand = 0;
unsigned long lastFaceFrame = 0;
unsigned long faceFrame = 0;
unsigned long nextIsabellaMoodAt = 0;
FaceMode isabellaMood = FACE_IDLE;
bool importedCharacterStateOverride = false;
bool cameraReady = false;
bool cameraBusy = false;
bool microphoneReady = false;
bool microphoneMonitor = false;
bool audioReady = false;
bool audioToneActive = false;
bool headFollowsState = true;
bool danceActive = false;
bool liveCameraActive = false;
bool oledInverted = false;
String otaUiState = "idle";
int otaUiProgress = 0;
bool rockyCharacterStateOverride = false;
bool leftMotorInvert = ROCKY_LEFT_MOTOR_INVERT;
bool rightMotorInvert = ROCKY_RIGHT_MOTOR_INVERT;
unsigned long microphoneMonitorUntil = 0;
unsigned long lastMicRead = 0;
unsigned long lastMicPrint = 0;
int microphoneLevel = 0;
unsigned long audioToneUntil = 0;
uint32_t audioTonePhase = 0;
uint32_t audioTonePhaseStep = 0;
int audioToneFrequency = 440;
int danceStep = 0;
unsigned long danceStepStarted = 0;

// ------------------------------ Declarations ------------------------------

void drawFace();
void syncImportedCharacterState();
bool drawImportedCharacter();
void drawEyes(int eyeY, int eyeH, bool narrow, bool round, bool lookingLeft, bool lookingRight);
void drawMouth();
void drawWiFiIndicator();
void drawOtaIndicator();
void onOtaUiEvent(const char *state, int progress);
void setOledInverted(bool enabled, const String &reason);
void drawPersonalityAccent();
void updateIsabellaMood();
void updateMicrophone();
void updateAudioTone();
void updateDance();
void applyStateHeadPose();
void startDance();
void stopDance(const String &reason);
void setFace(FaceMode mode, const String &eventText);
void setPersonality(const String &name);
void processSerialCommand(const String &command);
void runTest(const String &name);
void runMotorChannelTest(const String &side);
void setPan(int angle);
void stopMotors(const String &reason);
void driveMotors(const String &direction, int speed, uint32_t durationMs);
void setMotorChannel(int in1, int in2, int signedPwm, bool invert);
void startAccessPoint();
void connectStoredWiFi();
void maintainWiFi();
void setupRoutes();
void handleRoot();
void handleHealth();
void handleStatus();
void handleCommand();
void handleCameraJpg();
void handleCameraLive();
void handleWiFiForm();
void handleWiFiSave();
void handleNotFound();
String pageHeader(const String &title);
String htmlEscape(const String &value);
String faceName();
String personalityName();
String networkModeName();
String currentTimeText();
String motorDriverName();
String cameraStatusName();
bool initCamera();
bool initMicrophone();
String microphoneStatusName();
bool initAudio();
String audioStatusName();
void startAudioTone(int frequency, uint32_t durationMs);
void stopAudio(const String &reason);

// ------------------------------ Setup/loop ------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);
  randomSeed((uint32_t)micros());

  Wire.begin(ROCKY_OLED_SDA, ROCKY_OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, ROCKY_OLED_ADDRESS)) {
    Serial.println("OLED ERROR: check VCC/GND, SDA/SCL, address 0x3C or 0x3D");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(8, 20);
    display.println("Rocky XIAO booting");
    display.display();
  }

  // Servo signal only: use an external regulated 5 V servo supply and common GND.
  headServo.setPeriodHertz(50);
  headServo.attach(ROCKY_SERVO_PIN, 500, 2400);
  setPan(90);

  // DRV8833 has no separate enable pins. All four input pins are outputs.
  pinMode(ROCKY_LEFT_IN1, OUTPUT);
  pinMode(ROCKY_LEFT_IN2, OUTPUT);
  pinMode(ROCKY_RIGHT_IN1, OUTPUT);
  pinMode(ROCKY_RIGHT_IN2, OUTPUT);
  stopMotors("Boot safety stop");

  cameraReady = initCamera();
  microphoneReady = initMicrophone();
  audioReady = initAudio();

  preferences.begin("rocky-xiao", false);
  savedSsid = preferences.getString("ssid", "");
  savedPassword = preferences.getString("pass", "");
  leftMotorInvert = preferences.getBool("leftInv", ROCKY_LEFT_MOTOR_INVERT);
  rightMotorInvert = preferences.getBool("rightInv", ROCKY_RIGHT_MOTOR_INVERT);
  oledInverted = preferences.getBool("oledInv", false);
  display.invertDisplay(oledInverted);

  setupRoutes();
  // Network must exist before WebServer::begin() on recent ESP32 cores.
  connectStoredWiFi();
  server.begin();
  configTime(ROCKY_DEFAULT_TIMEZONE_OFFSET_SECONDS, 0, "pool.ntp.org", "time.nist.gov");
  otaService.setUiCallback(onOtaUiEvent);
  otaService.begin();

  setFace(FACE_IDLE, "Ready");
  nextIsabellaMoodAt = millis() + ROCKY_ISABELLA_MOOD_MIN_MS;

  Serial.println();
  Serial.println("Rocky XIAO modular bring-up ready");
  Serial.println("Hardware config: boards/xiao-esp32s3-sense/pinmap.h");
  Serial.println("Driver: DRV8833 direct-input PWM");
  Serial.println("Commands: help, status, test oled/servo/motor/left/right/wifi/camera/mic/audio");
  Serial.println("          face idle/listening/thinking/working/speaking/happy/laugh/curious/sad/surprised/error/sleep");
  Serial.println("          personality calm/rocky/engineer/spartan/isabella");
  Serial.println("          isabella neutral/shy/happy/concerned/listening/speaking/sleeping");
  Serial.println("          spartan guard/command/salute/listening/thinking/laughing/march/attack/victory/sleeping");
  Serial.println("          pan 0..180, head auto/on/off, motor forward/back/left/right/test/stop");
  Serial.println("          motor speed 0..255, motor invert left/right on/off, dance start/stop, mic monitor");
  Serial.println("          audio tone [Hz] [ms], audio stop");
  Serial.println("          oled invert on/off/toggle");
}

void loop() {
  server.handleClient();
  maintainWiFi();
  otaService.update();

  if (millis() - lastMotorCommand > ROCKY_MOTOR_COMMAND_TIMEOUT_MS) {
    stopMotors("Motor timeout safety stop");
  }

  if (personality == PERSONALITY_ISABELLA) updateIsabellaMood();
  updateMicrophone();
  updateAudioTone();
  updateDance();

  if (millis() - lastFaceFrame >= ROCKY_FACE_FRAME_MS) {
    lastFaceFrame = millis();
    faceFrame++;
    if (personality == PERSONALITY_ROCKY) RockyBodyLanguage::tickIntegration(millis());
    drawFace();
  }

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command.length() > 0) processSerialCommand(command);
  }
}

// ------------------------------ Command layer ------------------------------

void processSerialCommand(const String &command) {
  if (command.length() == 0) return;
  lastCommand = command;

  String lower = command;
  lower.toLowerCase();

  if (lower == "help") {
    Serial.println("test oled | test servo | test motor | test left | test right | test wifi | test camera | test mic | test audio");
    Serial.println("face idle|listening|thinking|speaking|working|happy|laugh|curious|sad|surprised|error|sleep");
    Serial.println("personality calm|rocky|engineer|spartan|isabella");
    Serial.println("rocky [idle|curious|greeting|listening|forward|back|pace|come|go|sleep|wake|dance]");
    Serial.println("isabella <state> | spartan <state>");
    Serial.println("pan 0..180");
    Serial.println("motor forward|back|left|right|test|stop");
    Serial.println("motor speed 0..255");
    Serial.println("motor invert left/right on/off | head auto/on/off | dance start/stop | mic monitor");
    Serial.println("audio tone [Hz] [ms] | audio stop");
    Serial.println("restart (reboot and check OTA manifest)");
    Serial.println("status");
    return;
  }

  if (lower == "restart" || lower == "reboot") {
    Serial.println("Restarting Deskbot; OTA-enabled builds will check GitHub Pages at boot");
    delay(250);
    ESP.restart();
    return;
  }

  if (lower.startsWith("test ")) {
    runTest(lower.substring(5));
    return;
  }

  if (lower == "status") {
    Serial.println("--- XIAO STATUS ---");
    Serial.print("Network: "); Serial.println(networkModeName());
    Serial.print("IP: "); Serial.println(provisioningAP ? WiFi.softAPIP() : WiFi.localIP());
    Serial.print("RSSI: "); Serial.println(provisioningAP ? 0 : WiFi.RSSI());
    Serial.print("Face: "); Serial.println(faceName());
    Serial.print("Personality: "); Serial.println(personalityName());
    Serial.print("Pan: "); Serial.println(panAngle);
    Serial.print("Motor driver: "); Serial.println(motorDriverName());
    Serial.print("Motor speed: "); Serial.println(motorSpeed);
    Serial.print("Camera: "); Serial.println(cameraStatusName());
    Serial.print("Microphone: "); Serial.println(microphoneStatusName());
    Serial.print("Mic level: "); Serial.println(microphoneLevel);
    Serial.print("Audio: "); Serial.println(audioStatusName());
    Serial.print("Head follows state: "); Serial.println(headFollowsState ? "on" : "off");
    Serial.print("Dance: "); Serial.println(danceActive ? "active" : "off");
    Serial.print("Motor invert L/R: "); Serial.print(leftMotorInvert); Serial.print("/"); Serial.println(rightMotorInvert);
    Serial.print("OLED invert: "); Serial.println(oledInverted ? "on" : "off");
    Serial.print("OTA state: "); Serial.println(otaService.stateName());
    Serial.print("OTA remote version: "); Serial.println(otaService.remoteVersion());
    Serial.print("OTA progress: "); Serial.print(otaService.progress()); Serial.println("%");
    Serial.print("Board ID: "); Serial.println(otaService.boardId());
    Serial.print("Firmware: "); Serial.println(APP_VERSION);
    Serial.println("OTA: checked once at boot; requires ota_target.h and a published newer manifest");
    Serial.print("Last event: "); Serial.println(lastEvent);
    return;
  }

  if (lower.startsWith("face ")) {
    String value = lower.substring(5);
    if (value == "idle") setFace(FACE_IDLE, "Face idle");
    else if (value == "listening") setFace(FACE_LISTENING, "Listening");
    else if (value == "thinking") setFace(FACE_THINKING, "Thinking");
    else if (value == "speaking") setFace(FACE_SPEAKING, "Speaking");
    else if (value == "working") setFace(FACE_WORKING, "Executing");
    else if (value == "happy") setFace(FACE_HAPPY, "Happy");
    else if (value == "laugh") setFace(FACE_LAUGH, "Laughing");
    else if (value == "curious") setFace(FACE_CURIOUS, "Curious");
    else if (value == "sad") setFace(FACE_SAD, "Sad");
    else if (value == "surprised") setFace(FACE_SURPRISED, "Surprised");
    else if (value == "error") setFace(FACE_ERROR, "Error");
    else if (value == "sleep") setFace(FACE_SLEEP, "Sleeping");
    else Serial.println("Unknown face");
    return;
  }

  if (lower.startsWith("personality ")) {
    setPersonality(lower.substring(12));
    return;
  }

  if (lower == "rocky") {
    setPersonality("rocky");
    return;
  }
  if (lower.startsWith("rocky ")) {
    setPersonality("rocky");
    const String rockyState = lower.substring(6);
    RockyBodyLanguage::BodyState targetState;
    if (RockyBodyLanguage::bodyStateFromName(rockyState, targetState)) {
      RockyBodyLanguage::setIntegrationState(rockyState);
      rockyCharacterStateOverride = true;
      lastEvent = String("Rocky body state: ") + RockyBodyLanguage::stateName();
      Serial.println(lastEvent);
      drawFace();
    } else {
      Serial.println("Rocky states: idle, curious, greeting, listening, forward, back, pace, come, go, sleep, wake, dance");
    }
    return;
  }

  if (lower == "isabella") {
    setPersonality("isabella");
    return;
  }
  if (lower.startsWith("isabella ")) {
    setPersonality("isabella");
    const String stateName = lower.substring(9);
    if (setIsabellaStateByName(stateName)) {
      importedCharacterStateOverride = true;
      lastEvent = "Isabella state: " + stateName;
      drawFace();
    } else {
      printIsabellaHelp();
    }
    return;
  }
  if (lower == "spartan") {
    setPersonality("spartan");
    return;
  }
  if (lower.startsWith("spartan ")) {
    setPersonality("spartan");
    const String stateName = lower.substring(8);
    if (setSpartanStateByName(stateName)) {
      importedCharacterStateOverride = true;
      lastEvent = "Spartan state: " + stateName;
      drawFace();
    } else {
      printSpartanHelp();
    }
    return;
  }

  if (lower.startsWith("oled invert")) {
    String value = lower.substring(11);
    value.trim();
    if (value == "toggle") setOledInverted(!oledInverted, "OLED invert toggled");
    else if (value == "on" || value == "1" || value == "true") setOledInverted(true, "OLED inverted");
    else if (value == "off" || value == "0" || value == "false") setOledInverted(false, "OLED normal");
    else Serial.println("Use: oled invert on/off/toggle");
    return;
  }
  if (lower.startsWith("pan ")) {
    setPan(constrain(lower.substring(4).toInt(), ROCKY_SERVO_MIN_ANGLE, ROCKY_SERVO_MAX_ANGLE));
    return;
  }

  if (lower.startsWith("motor speed ")) {
    motorSpeed = constrain(lower.substring(12).toInt(), 0, 255);
    lastEvent = "Motor speed " + String(motorSpeed);
    Serial.println(lastEvent);
    return;
  }

  if (lower.startsWith("motor invert ")) {
    String value = lower.substring(13);
    int split = value.indexOf(' ');
    if (split < 0) {
      Serial.println("Use: motor invert left/right on/off");
      return;
    }
    String side = value.substring(0, split);
    String setting = value.substring(split + 1);
    bool enabled = (setting == "on" || setting == "1" || setting == "true");
    if (side == "left") {
      leftMotorInvert = enabled;
      preferences.putBool("leftInv", leftMotorInvert);
    } else if (side == "right") {
      rightMotorInvert = enabled;
      preferences.putBool("rightInv", rightMotorInvert);
    } else {
      Serial.println("Use left or right");
      return;
    }
    lastEvent = "Motor invert " + side + " " + (enabled ? "on" : "off");
    Serial.println(lastEvent);
    return;
  }

  if (lower == "head auto" || lower == "head on") {
    headFollowsState = true;
    lastEvent = "Head follows interaction state";
    Serial.println(lastEvent);
    return;
  }
  if (lower == "head off") {
    headFollowsState = false;
    lastEvent = "Head state following off";
    Serial.println(lastEvent);
    return;
  }
  if (lower == "dance start" || lower == "dance rocky" || lower == "dance isabella") {
    startDance();
    return;
  }
  if (lower == "dance stop") {
    stopDance("Dance stopped");
    return;
  }
  if (lower == "mic monitor" || lower == "mic on") {
    microphoneMonitor = true;
    microphoneMonitorUntil = millis() + 60000UL;
    setFace(FACE_LISTENING, "Microphone monitor");
    Serial.println("Microphone monitor active for 60 seconds");
    return;
  }
  if (lower == "mic off") {
    microphoneMonitor = false;
    microphoneLevel = 0;
    setFace(FACE_IDLE, "Microphone monitor off");
    return;
  }

  if (lower == "audio stop") {
    stopAudio("Audio stopped");
    return;
  }

  if (lower.startsWith("audio tone")) {
    String args = lower.substring(10);
    args.trim();
    int split = args.indexOf(' ');
    int frequency = split < 0 ? args.toInt() : args.substring(0, split).toInt();
    int duration = split < 0 ? 1500 : args.substring(split + 1).toInt();
    if (frequency <= 0) frequency = 440;
    if (duration <= 0) duration = 1500;
    startAudioTone(constrain(frequency, 100, 4000), constrain(duration, 100, 10000));
    return;
  }

  if (lower == "motor forward") driveMotors("forward", motorSpeed, 700);
  else if (lower == "motor back") driveMotors("back", motorSpeed, 700);
  else if (lower == "motor left") driveMotors("left", motorSpeed, 500);
  else if (lower == "motor right") driveMotors("right", motorSpeed, 500);
  else if (lower == "motor test") runTest("motor");
  else if (lower == "motor stop") stopMotors("Manual motor stop");
  else Serial.println("Unknown command; type help");
}

void runTest(const String &name) {
  if (name == "oled") {
    Serial.println("TEST OLED: cycling face states");
    setFace(FACE_HAPPY, "OLED test happy");
    delay(500);
    setFace(FACE_THINKING, "OLED test thinking");
    delay(700);
    setFace(FACE_WORKING, "OLED test working");
    delay(700);
    setFace(FACE_SPEAKING, "OLED test speaking");
    delay(700);
    setFace(FACE_IDLE, "OLED test complete");
    return;
  }
  if (name == "servo") {
    Serial.println("TEST SERVO: 90 -> 60 -> 120 -> 90");
    setPan(90); delay(400);
    setPan(60); delay(500);
    setPan(120); delay(500);
    setPan(90);
    return;
  }
  if (name == "motor") {
    Serial.println("TEST MOTOR: forward then stop; keep robot lifted");
    driveMotors("forward", motorSpeed, 500);
    delay(650);
    stopMotors("Motor test complete");
    return;
  }
  if (name == "left" || name == "right") {
    runMotorChannelTest(name);
    return;
  }
  if (name == "wifi") {
    Serial.print("Wi-Fi: "); Serial.println(networkModeName());
    Serial.print("IP: "); Serial.println(provisioningAP ? WiFi.softAPIP() : WiFi.localIP());
    Serial.print("RSSI: "); Serial.println(provisioningAP ? 0 : WiFi.RSSI());
    return;
  }
  if (name == "camera") {
    Serial.print("Camera: "); Serial.println(cameraStatusName());
    Serial.println(cameraReady ? "Open /camera.jpg from the dashboard device IP" : "Camera is not initialized");
    return;
  }
  if (name == "mic") {
    if (!microphoneReady) {
      Serial.println("MIC ERROR: microphone is not initialized");
      return;
    }
    microphoneMonitor = true;
    microphoneMonitorUntil = millis() + 60000UL;
    setFace(FACE_LISTENING, "Microphone test");
    Serial.println("MIC TEST: speak near the Sense microphone for 60 seconds");
    return;
  }
  if (name == "audio") {
    if (!audioReady) {
      Serial.println("AUDIO ERROR: MAX98357A I2S output is not initialized");
      return;
    }
    Serial.println("AUDIO TEST: 440 Hz tone for 1.5 seconds; start with low volume");
    startAudioTone(440, 1500);
    return;
  }
  Serial.println("Unknown test. Use: test oled, test servo, test motor, test left, test right, test wifi, test camera, test mic, test audio");
}

void runMotorChannelTest(const String &side) {
  Serial.print("TEST MOTOR CHANNEL: "); Serial.println(side);
  Serial.println("Keep the robot lifted. This runs only one N90 for 400 ms.");
  if (side == "left") {
    setMotorChannel(ROCKY_LEFT_IN1, ROCKY_LEFT_IN2, motorSpeed, leftMotorInvert);
  } else {
    setMotorChannel(ROCKY_RIGHT_IN1, ROCKY_RIGHT_IN2, motorSpeed, rightMotorInvert);
  }
  delay(400);
  stopMotors("Single motor channel test complete");
}

void setFace(FaceMode mode, const String &eventText) {
  faceMode = mode;
  importedCharacterStateOverride = false;
  rockyCharacterStateOverride = false;
  lastEvent = eventText;
  if (personality == PERSONALITY_ISABELLA && mode == FACE_IDLE) isabellaMood = FACE_IDLE;
  applyStateHeadPose();
  Serial.print("FACE: "); Serial.println(eventText);
  drawFace();
}

void setPersonality(const String &name) {
  String value = name;
  value.toLowerCase();
  if (value == "calm") personality = PERSONALITY_CALM;
  else if (value == "rocky" || value == "musical") personality = PERSONALITY_ROCKY;
  else if (value == "engineer") personality = PERSONALITY_ENGINEER;
  else if (value == "spartan") personality = PERSONALITY_SPARTAN;
  else if (value == "isabella" || value == "jolly" || value == "anime") personality = PERSONALITY_ISABELLA;
  else {
    Serial.println("Unknown personality; use calm, rocky, engineer, spartan, or isabella");
    return;
  }

  isabellaMood = FACE_IDLE;
  importedCharacterStateOverride = false;
  rockyCharacterStateOverride = false;
  nextIsabellaMoodAt = millis() + ROCKY_ISABELLA_MOOD_MIN_MS;
  if (personality == PERSONALITY_ROCKY) {
    RockyBodyLanguage::rockyPersonality = true;
    RockyBodyLanguage::playing = true;
    RockyBodyLanguage::setIntegrationState("idle");
  } else {
    RockyBodyLanguage::rockyPersonality = false;
  }
  lastEvent = "Personality: " + personalityName();
  Serial.println(lastEvent);
  drawFace();
}

void syncImportedCharacterState() {
  if (personality == PERSONALITY_ISABELLA) {
    if (importedCharacterStateOverride) {
      isabellaFrame = static_cast<uint8_t>(faceFrame);
      return;
    }
    IsabellaState target = ISABELLA_NEUTRAL;
    switch (faceMode) {
      case FACE_LISTENING: target = ISABELLA_LISTENING; break;
      case FACE_SPEAKING: target = ISABELLA_SPEAKING; break;
      case FACE_HAPPY: target = ISABELLA_HAPPY; break;
      case FACE_LAUGH: target = ISABELLA_LAUGHING; break;
      case FACE_SURPRISED: target = ISABELLA_SURPRISED; break;
      case FACE_SAD: target = ISABELLA_SAD; break;
      case FACE_ERROR: target = ISABELLA_CONCERNED; break;
      case FACE_THINKING: target = ISABELLA_THINKING; break;
      case FACE_WORKING: target = ISABELLA_THINKING; break;
      case FACE_CURIOUS: target = ISABELLA_CURIOUS; break;
      case FACE_SLEEP: target = ISABELLA_SLEEPING; break;
      default: target = ISABELLA_NEUTRAL; break;
    }
    if (isabellaState != target) setIsabellaState(target);
    isabellaFrame = static_cast<uint8_t>(faceFrame);
  } else if (personality == PERSONALITY_SPARTAN) {
    if (importedCharacterStateOverride) {
      spartanFrame = static_cast<uint8_t>(faceFrame);
      return;
    }
    switch (faceMode) {
      case FACE_LISTENING: setSpartanState(SPARTAN_LISTENING); break;
      case FACE_THINKING: setSpartanState(SPARTAN_THINKING); break;
      case FACE_SPEAKING:
      case FACE_WORKING: setSpartanState(SPARTAN_COMMAND); break;
      case FACE_HAPPY: setSpartanState(SPARTAN_VICTORY); break;
      case FACE_LAUGH: setSpartanState(SPARTAN_LAUGHING); break;
      case FACE_SLEEP: setSpartanState(SPARTAN_SLEEPING); break;
      case FACE_CURIOUS: setSpartanState(SPARTAN_THINKING); break;
      default: setSpartanState(SPARTAN_GUARD); break;
    }
    spartanFrame = static_cast<uint8_t>(faceFrame);
  } else if (personality == PERSONALITY_ROCKY) {
    if (rockyCharacterStateOverride) return;
    switch (faceMode) {
      case FACE_LISTENING: RockyBodyLanguage::setIntegrationState("listening"); break;
      case FACE_THINKING: RockyBodyLanguage::setIntegrationState("curious"); break;
      case FACE_SPEAKING: RockyBodyLanguage::setIntegrationState("greeting"); break;
      case FACE_WORKING: RockyBodyLanguage::setIntegrationState("pace"); break;
      case FACE_HAPPY: RockyBodyLanguage::setIntegrationState("greeting"); break;
      case FACE_LAUGH: RockyBodyLanguage::setIntegrationState("dance"); break;
      case FACE_CURIOUS: RockyBodyLanguage::setIntegrationState("curious"); break;
      case FACE_SAD: RockyBodyLanguage::setIntegrationState("sleeping"); break;
      case FACE_SURPRISED: RockyBodyLanguage::setIntegrationState("wave"); break;
      case FACE_ERROR: RockyBodyLanguage::setIntegrationState("go back"); break;
      case FACE_SLEEP: RockyBodyLanguage::setIntegrationState("sleeping"); break;
      default: RockyBodyLanguage::setIntegrationState("idle"); break;
    }
  }
}

bool drawImportedCharacter() {
  syncImportedCharacterState();
  if (personality == PERSONALITY_ISABELLA) {
    drawIsabella();
    return true;
  }
  if (personality == PERSONALITY_SPARTAN) {
    drawSpartan();
    return true;
  }
  if (personality == PERSONALITY_ROCKY) {
    RockyBodyLanguage::renderIntegrationFrame();
    return true;
  }
  return false;
}

void setPan(int angle) {
  panAngle = constrain(angle, ROCKY_SERVO_MIN_ANGLE, ROCKY_SERVO_MAX_ANGLE);
  headServo.write(panAngle);
  lastEvent = "Pan " + String(panAngle) + " deg";
  Serial.println(lastEvent);
}

// ------------------------------ DRV8833 motor layer ------------------------------

void setMotorChannel(int in1, int in2, int signedPwm, bool invert) {
  if (invert) signedPwm = -signedPwm;
  int pwm = constrain(abs(signedPwm), 0, 255);
  if (signedPwm > 0) {
    analogWrite(in1, pwm);
    analogWrite(in2, 0);
  } else if (signedPwm < 0) {
    analogWrite(in1, 0);
    analogWrite(in2, pwm);
  } else {
    analogWrite(in1, 0);
    analogWrite(in2, 0);
  }
}

void driveMotors(const String &direction, int speed, uint32_t durationMs) {
  int pwm = constrain(speed, 0, 255);
  int left = 0;
  int right = 0;

  if (direction == "forward") {
    left = pwm; right = pwm;
  } else if (direction == "back") {
    left = -pwm; right = -pwm;
  } else if (direction == "left") {
    left = -pwm; right = pwm;
  } else if (direction == "right") {
    left = pwm; right = -pwm;
  } else {
    stopMotors("Unknown motor direction");
    return;
  }

  setMotorChannel(ROCKY_LEFT_IN1, ROCKY_LEFT_IN2, left, leftMotorInvert);
  setMotorChannel(ROCKY_RIGHT_IN1, ROCKY_RIGHT_IN2, right, rightMotorInvert);
  lastMotorCommand = millis();
  lastEvent = "Motor " + direction;
  setFace(FACE_HAPPY, lastEvent);
  Serial.println(lastEvent);
  (void)durationMs; // safety timeout is the hard stop boundary
}

void stopMotors(const String &reason) {
  setMotorChannel(ROCKY_LEFT_IN1, ROCKY_LEFT_IN2, 0, false);
  setMotorChannel(ROCKY_RIGHT_IN1, ROCKY_RIGHT_IN2, 0, false);
  lastMotorCommand = millis();
  lastEvent = reason;
}

// ------------------------------ Wi-Fi layer ------------------------------

void connectStoredWiFi() {
  if (savedSsid.length() == 0) {
    startAccessPoint();
    return;
  }

  provisioningAP = false;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("deskbot-xiao");
  WiFi.begin(savedSsid.c_str(), savedPassword.c_str());
  Serial.print("Connecting to Wi-Fi: "); Serial.println(savedSsid);

  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 12000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WIFI OK IP: "); Serial.println(WiFi.localIP());
    Serial.print("DASHBOARD: http://"); Serial.println(WiFi.localIP());
    Serial.println("Open that URL from a phone/computer on the same 2.4 GHz Wi-Fi.");
    lastEvent = "Wi-Fi connected";
  } else {
    Serial.println("Wi-Fi failed; starting setup AP");
    startAccessPoint();
  }
}

void startAccessPoint() {
  provisioningAP = true;
  WiFi.disconnect(true, true);
  delay(200);
  WiFi.mode(WIFI_AP);
  IPAddress apIp(192, 168, 4, 1);
  IPAddress apGateway(192, 168, 4, 1);
  IPAddress apMask(255, 255, 255, 0);
  WiFi.softAPConfig(apIp, apGateway, apMask);
  WiFi.softAP("Rocky-XIAO-Setup");
  Serial.print("SETUP AP: Rocky-XIAO-Setup IP: "); Serial.println(WiFi.softAPIP());
  Serial.println("DASHBOARD: http://192.168.4.1");
  Serial.println("Connect to Rocky-XIAO-Setup, open http://192.168.4.1, and save 2.4 GHz Wi-Fi settings.");
  lastEvent = "Wi-Fi setup AP active";
}

void maintainWiFi() {
  if (provisioningAP) return;
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastWiFiAttempt < ROCKY_WIFI_RETRY_MS) return;

  lastWiFiAttempt = millis();
  Serial.println("Wi-Fi reconnect attempt");
  WiFi.disconnect();
  WiFi.begin(savedSsid.c_str(), savedPassword.c_str());
}

bool initMicrophone() {
  microphone.setPinsPdmRx(ROCKY_PDM_MIC_CLOCK, ROCKY_PDM_MIC_DATA);
  if (!microphone.begin(I2S_MODE_PDM_RX, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("MIC ERROR: PDM microphone initialization failed");
    return false;
  }
  Serial.println("MIC OK: XIAO Sense PDM microphone at 16 kHz");
  return true;
}

String microphoneStatusName() {
  if (!microphoneReady) return "not_ready";
  if (microphoneMonitor) return "monitoring";
  return "ready";
}

bool initAudio() {
  // Use I2S controller 1 so the Sense PDM microphone can keep its RX channel.
  audioOutput.setPins(ROCKY_AUDIO_BCLK, ROCKY_AUDIO_LRCK, ROCKY_AUDIO_DATA);
  if (!audioOutput.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("AUDIO ERROR: MAX98357A I2S initialization failed");
    return false;
  }
  Serial.println("AUDIO OK: MAX98357A I2S output at 16 kHz");
  return true;
}

String audioStatusName() {
  if (!audioReady) return "not_ready";
  return audioToneActive ? "tone" : "ready";
}

void startAudioTone(int frequency, uint32_t durationMs) {
  if (!audioReady) {
    Serial.println("AUDIO ERROR: initialize MAX98357A output first");
    return;
  }
  audioToneFrequency = constrain(frequency, 100, 4000);
  audioTonePhase = 0;
  audioTonePhaseStep = (uint32_t)(((uint64_t)audioToneFrequency << 32) / 16000ULL);
  audioToneUntil = millis() + durationMs;
  audioToneActive = true;
  setFace(FACE_SPEAKING, "Audio tone");
  Serial.print("AUDIO: tone ");
  Serial.print(audioToneFrequency);
  Serial.print(" Hz for ");
  Serial.print(durationMs);
  Serial.println(" ms");
}

void stopAudio(const String &reason) {
  audioToneActive = false;
  audioToneUntil = 0;
  lastEvent = reason;
  if (faceMode == FACE_SPEAKING) setFace(FACE_IDLE, reason);
  else Serial.println(reason);
}

void updateAudioTone() {
  if (!audioReady || !audioToneActive) return;
  if ((int32_t)(millis() - audioToneUntil) >= 0) {
    stopAudio("Audio tone complete");
    return;
  }

  int16_t stereoSamples[64 * 2];
  for (size_t i = 0; i < 64; ++i) {
    const int16_t sample = (audioTonePhase & 0x80000000UL) ? 1800 : -1800;
    stereoSamples[i * 2] = sample;
    stereoSamples[i * 2 + 1] = sample;
    audioTonePhase += audioTonePhaseStep;
  }
  audioOutput.write(reinterpret_cast<const uint8_t *>(stereoSamples), sizeof(stereoSamples));
}

void updateMicrophone() {
  if (!microphoneReady || !microphoneMonitor) return;
  if (millis() >= microphoneMonitorUntil) {
    microphoneMonitor = false;
    microphoneLevel = 0;
    setFace(FACE_IDLE, "Microphone monitor complete");
    return;
  }
  if (millis() - lastMicRead < 40 || microphone.available() <= 0) return;
  lastMicRead = millis();

  int16_t samples[256];
  size_t bytes = microphone.readBytes((char *)samples, sizeof(samples));
  size_t count = bytes / sizeof(int16_t);
  if (count == 0) return;

  uint32_t total = 0;
  for (size_t i = 0; i < count; i++) {
    int32_t sample = samples[i];
    if (sample < 0) sample = -sample;
    total += (uint32_t)sample;
  }
  uint32_t average = total / count;
  // The onboard PDM mic has a small idle noise floor. Remove it first, then
  // smooth the meter so the dashboard shows voice/activity rather than raw
  // sample-to-sample jitter. This is an amplitude meter, not speech-to-text.
  const uint32_t noiseFloor = 180;
  uint32_t aboveFloor = average > noiseFloor ? average - noiseFloor : 0;
  int rawLevel = constrain((int)(aboveFloor / 110), 0, 99);
  static int smoothedLevel = 0;
  smoothedLevel = (smoothedLevel * 7 + rawLevel * 3) / 10;
  microphoneLevel = smoothedLevel < 2 ? 0 : smoothedLevel;
  if (millis() - lastMicPrint >= 250) {
    lastMicPrint = millis();
    Serial.print("MIC level: "); Serial.println(microphoneLevel);
  }
}

void applyStateHeadPose() {
  if (!headFollowsState || danceActive) return;
  int target = 90;
  switch (faceMode) {
    case FACE_LISTENING: target = 90; break;
    case FACE_THINKING: target = 118; break;
    case FACE_WORKING: target = 62; break;
    case FACE_CURIOUS: target = 125; break;
    case FACE_SAD: target = 72; break;
    case FACE_ERROR: target = 58; break;
    default: target = 90; break;
  }
  panAngle = constrain(target, ROCKY_SERVO_MIN_ANGLE, ROCKY_SERVO_MAX_ANGLE);
  headServo.write(panAngle);
}

void startDance() {
  danceActive = true;
  danceStep = 0;
  danceStepStarted = 0;
  lastEvent = "Dance started";
  Serial.println(lastEvent);
}

void stopDance(const String &reason) {
  danceActive = false;
  stopMotors(reason);
  panAngle = 90;
  headServo.write(panAngle);
  setFace(FACE_IDLE, reason);
}

void updateDance() {
  if (!danceActive) return;
  const unsigned long stepMs = 650;
  if (danceStepStarted != 0 && millis() - danceStepStarted < stepMs) return;
  danceStepStarted = millis();

  switch (danceStep) {
    case 0: setPan(90); setFace(FACE_HAPPY, "Dance center"); stopMotors("Dance pause"); break;
    case 1: setPan(58); setFace(FACE_CURIOUS, "Dance look left"); driveMotors("left", motorSpeed, stepMs); break;
    case 2: setPan(122); setFace(FACE_HAPPY, "Dance look right"); driveMotors("right", motorSpeed, stepMs); break;
    case 3: setPan(90); setFace(FACE_SURPRISED, "Dance bounce"); driveMotors("forward", motorSpeed, stepMs); break;
    case 4: setPan(70); setFace(FACE_LAUGH, "Dance sway left"); driveMotors("left", motorSpeed, stepMs); break;
    case 5: setPan(110); setFace(FACE_LAUGH, "Dance sway right"); driveMotors("right", motorSpeed, stepMs); break;
    case 6: setPan(90); setFace(FACE_HAPPY, "Dance finish"); driveMotors("back", motorSpeed, stepMs); break;
    default: stopDance("Dance complete"); return;
  }
  danceStep++;
}

bool initCamera() {
#if ROCKY_CAMERA_ENABLED
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = ROCKY_CAMERA_Y2;
  config.pin_d1 = ROCKY_CAMERA_Y3;
  config.pin_d2 = ROCKY_CAMERA_Y4;
  config.pin_d3 = ROCKY_CAMERA_Y5;
  config.pin_d4 = ROCKY_CAMERA_Y6;
  config.pin_d5 = ROCKY_CAMERA_Y7;
  config.pin_d6 = ROCKY_CAMERA_Y8;
  config.pin_d7 = ROCKY_CAMERA_Y9;
  config.pin_xclk = ROCKY_CAMERA_XCLK;
  config.pin_pclk = ROCKY_CAMERA_PCLK;
  config.pin_vsync = ROCKY_CAMERA_VSYNC;
  config.pin_href = ROCKY_CAMERA_HREF;
  config.pin_sccb_sda = ROCKY_CAMERA_SIOD;
  config.pin_sccb_scl = ROCKY_CAMERA_SIOC;
  config.pin_pwdn = ROCKY_CAMERA_PWDN;
  config.pin_reset = ROCKY_CAMERA_RESET;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = ROCKY_CAMERA_FRAME_SIZE;
  config.jpeg_quality = ROCKY_CAMERA_JPEG_QUALITY;
  config.fb_count = psramFound() ? 2 : 1;
  config.grab_mode = psramFound() ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("CAMERA ERROR: init failed 0x%x\n", err);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor && sensor->id.PID == OV3660_PID) {
    sensor->set_vflip(sensor, 1);
    sensor->set_brightness(sensor, 1);
    sensor->set_saturation(sensor, -2);
  }
  Serial.println("CAMERA OK: XIAO Sense JPEG snapshot ready");
  return true;
#else
    Serial.println("CAMERA DISABLED in boards/xiao-esp32s3-sense/pinmap.h");
  return false;
#endif
}

void handleCameraJpg() {
  if (!cameraReady) {
    server.send(503, "text/plain", "Camera is not initialized");
    return;
  }
  if (cameraBusy) {
    server.send(429, "text/plain", "Camera busy; retry");
    return;
  }

  cameraBusy = true;
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    cameraBusy = false;
    server.send(503, "text/plain", "Camera capture failed");
    return;
  }

  // Some ESP32 WebServer versions return NetworkClient&, while older or
  // alternate WebServer libraries return a WiFiClient temporary. Store a
  // client value instead of binding a non-const reference to the result.
  WiFiClient client = server.client();
  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: image/jpeg\r\n");
  client.print("Content-Length: ");
  client.print(fb->len);
  client.print("\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n");
  client.write(fb->buf, fb->len);
  esp_camera_fb_return(fb);
  cameraBusy = false;
}

void handleCameraLive() {
  String html = pageHeader("Live XIAO Camera");
  html += "<h1>Live camera</h1><p class='muted'>Low-rate live mode: the browser requests a fresh JPEG snapshot several times per second. This keeps the XIAO responsive for the OLED, servo, motors, and microphone.</p>";
  html += "<img id='live' style='width:100%;max-width:640px;border-radius:10px;background:#080a0e' alt='Live camera preview'>";
  html += "<p><button onclick='startLive()'>Start</button><button onclick='stopLive()'>Stop</button><a class='button' href='/'>Back to dashboard</a></p>";
  html += "<pre id='fps'>stopped</pre>";
  html += "<script>let timer=null,count=0,last=Date.now();function tick(){let img=document.getElementById('live');img.src='/camera.jpg?live='+Date.now();count++;let now=Date.now();if(now-last>1000){document.getElementById('fps').textContent='refreshes/sec: '+count;count=0;last=now}}function startLive(){stopLive();tick();timer=setInterval(tick,300)}function stopLive(){if(timer){clearInterval(timer);timer=null}document.getElementById('fps').textContent='stopped'}startLive();</script></body></html>";
  server.send(200, "text/html", html);
}

// ------------------------------ HTTP/dashboard layer ------------------------------

void setupRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/health", HTTP_GET, handleHealth);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/camera.jpg", HTTP_GET, handleCameraJpg);
  server.on("/camera/live", HTTP_GET, handleCameraLive);
  server.on("/cmd", HTTP_GET, handleCommand);
  server.on("/wifi", HTTP_GET, handleWiFiForm);
  server.on("/wifi", HTTP_POST, handleWiFiSave);
  server.onNotFound(handleNotFound);
}

void handleRoot() {
  String html = pageHeader("Rocky XIAO Desk Buddy");
  html += "<h1>Rocky XIAO Desk Buddy</h1>";
  html += "<p class='muted'>Modular XIAO bring-up: OLED, servo, DRV8833, Wi-Fi</p>";
  html += "<section><h2>Status</h2><pre id='status'>Loading...</pre><button onclick='refreshStatus()'>Refresh</button></section>";
  html += "<section><h2>Face states</h2>";
  const char *faces[] = {"idle", "listening", "thinking", "speaking", "working", "happy", "laugh", "curious", "sad", "surprised", "error", "sleep"};
  for (const char *face : faces) {
    html += "<button onclick=\"cmd('face " + String(face) + "')\">" + String(face) + "</button>";
  }
  html += "<p class='muted'>Thinking = considering; Working = executing a task. They now use different eyes and labels.</p></section>";
  html += "<section><h2>Personalities</h2>";
  html += "<button onclick=\"cmd('personality calm')\">Calm</button>";
  html += "<button onclick=\"cmd('personality rocky')\">Rocky</button>";
  html += "<button onclick=\"cmd('personality engineer')\">Engineer</button>";
  html += "<button onclick=\"cmd('personality spartan')\">Spartan</button>";
  html += "<button class='isabella' onclick=\"cmd('personality isabella')\">Isabella</button></section>";
  html += "<section><h2>OLED display</h2><p class='muted'>Invert the monochrome OLED palette without reflashing. The setting is stored on the board.</p><button onclick=\"cmd('oled invert on')\">Invert ON</button><button onclick=\"cmd('oled invert off')\">Invert OFF</button><button onclick=\"cmd('oled invert toggle')\">Toggle</button></section>";
  html += "<section><h2>Pan servo</h2>";
  html += "<button onclick=\"cmd('pan 0')\">0°</button><button onclick=\"cmd('pan 90')\">90°</button><button onclick=\"cmd('pan 180')\">180°</button>";
  html += "<input id='angle' type='number' min='0' max='180' value='90'><button onclick=\"cmd('pan '+document.getElementById('angle').value)\">Move</button></section>";
  html += "<section><h2>Head and dance</h2><button onclick=\"cmd('head auto')\">Head follows state</button><button onclick=\"cmd('head off')\">Head fixed</button><button onclick=\"cmd('dance start')\">Dance</button><button class='stop' onclick=\"cmd('dance stop')\">Stop dance</button></section>";
  html += "<section><h2>DRV8833 motors</h2><p class='warn'>Keep the robot lifted. Motor supply and servo supply are external.</p>";
  html += "<button onclick=\"cmd('motor forward')\">Forward</button><button onclick=\"cmd('motor back')\">Back</button>";
  html += "<button onclick=\"cmd('motor left')\">Left</button><button onclick=\"cmd('motor right')\">Right</button>";
  html += "<button class='stop' onclick=\"cmd('motor stop')\">STOP</button>";
  html += "<input id='speed' type='number' min='0' max='255' value='100'><button onclick=\"cmd('motor speed '+document.getElementById('speed').value)\">Set speed</button><br><button onclick=\"cmd('motor invert left on')\">Left invert ON</button><button onclick=\"cmd('motor invert left off')\">Left invert OFF</button><button onclick=\"cmd('motor invert right on')\">Right invert ON</button><button onclick=\"cmd('motor invert right off')\">Right invert OFF</button></section>";
  html += "<section><h2>Single-function tests</h2><button onclick=\"cmd('test oled')\">Test OLED</button><button onclick=\"cmd('test servo')\">Test servo</button><button onclick=\"cmd('test left')\">Test left motor</button><button onclick=\"cmd('test right')\">Test right motor</button><button onclick=\"cmd('test motor')\">Test motors</button><button onclick=\"cmd('test wifi')\">Test Wi-Fi</button><button onclick=\"cmd('test mic')\">Test microphone</button><button onclick=\"cmd('mic off')\">Stop mic</button><button onclick=\"cmd('test audio')\">Test audio tone</button><button class='stop' onclick=\"cmd('audio stop')\">Stop audio</button></section>";
  html += "<section><h2>Camera</h2><p class='muted'>Snapshot or low-rate live preview. Live mode refreshes JPEG frames without blocking the rest of the robot.</p><img id='camera' style='width:100%;max-width:640px;border-radius:10px;background:#080a0e' alt='Camera preview'><br><button onclick='refreshCamera()'>Snapshot</button><a class='button' href='/camera/live' target='_blank'>Live mode</a><a class='button' href='/camera.jpg' target='_blank'>Open JPEG</a></section>";
  html += "<section><h2>Firmware and OTA</h2><div id='otaBanner' class='ota idle'>OTA status: loading</div><pre id='ota'>Loading...</pre><button onclick=\"cmd('restart')\">Restart and check OTA</button><p class='muted'>After GitHub Actions publishes a newer version, this button reboots the board. The OLED and Serial Monitor show checking, downloading, progress, and rebooting.</p></section>";
  html += "<section><h2>Network</h2><a class='button' href='/wifi'>Configure Wi-Fi</a></section>";
  html += "<script>async function cmd(c){try{let r=await fetch('/cmd?c='+encodeURIComponent(c));let t=await r.text();document.getElementById('status').textContent=t;await refreshStatus()}catch(e){document.getElementById('status').textContent='Command failed: '+e}} function otaText(s){let text='firmware: '+s.firmware+'\\nmanifest configured: '+s.otaConfigured+'\\nstate: '+s.otaState+'\\nremote version: '+(s.otaRemoteVersion||'unknown')+'\\nprogress: '+s.otaProgress+'%';if(s.otaError)text+='\\nerror: '+s.otaError;return text} function otaBanner(s){let el=document.getElementById('otaBanner');el.className='ota '+s.otaState;let label=s.otaState.replaceAll('_',' ');el.textContent='OTA status: '+label+(s.otaRemoteVersion?' | remote '+s.otaRemoteVersion:'')+(s.otaState==='downloading'?' | '+s.otaProgress+'%':'')} async function refreshStatus(){try{let r=await fetch('/api/status?ts='+Date.now());if(!r.ok)throw new Error('HTTP '+r.status);let s=await r.json();document.getElementById('status').textContent=JSON.stringify(s,null,2);document.getElementById('ota').textContent=otaText(s);otaBanner(s)}catch(e){document.getElementById('status').textContent='Dashboard cannot reach device: '+e}} function refreshCamera(){if(document.getElementById('camera'))document.getElementById('camera').src='/camera.jpg?ts='+Date.now()} refreshStatus();refreshCamera();setInterval(refreshStatus,5000);</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleHealth() {
  server.send(200, "text/plain", "deskbot-ok");
}

void handleStatus() {
  String json = "{";
  json += "\"network\":\"" + htmlEscape(networkModeName()) + "\",";
  json += "\"ip\":\"" + (provisioningAP ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\",";
  json += "\"ssid\":\"" + htmlEscape(savedSsid) + "\",";
  json += "\"rssi\":" + String(provisioningAP ? 0 : WiFi.RSSI()) + ",";
  json += "\"face\":\"" + faceName() + "\",";
  json += "\"personality\":\"" + personalityName() + "\",";
  json += "\"pan\":" + String(panAngle) + ",";
  json += "\"motorDriver\":\"" + motorDriverName() + "\",";
  json += "\"motorSpeed\":" + String(motorSpeed) + ",";
  json += "\"camera\":\"" + cameraStatusName() + "\",";
  json += "\"microphone\":\"" + microphoneStatusName() + "\",";
  json += "\"micLevel\":" + String(microphoneLevel) + ",";
  json += "\"audio\":\"" + audioStatusName() + "\",";
  json += "\"headFollowsState\":" + String(headFollowsState ? "true" : "false") + ",";
  json += "\"dance\":" + String(danceActive ? "true" : "false") + ",";
  json += "\"motorInvertLeft\":" + String(leftMotorInvert ? "true" : "false") + ",";
  json += "\"motorInvertRight\":" + String(rightMotorInvert ? "true" : "false") + ",";
  json += "\"oledInverted\":" + String(oledInverted ? "true" : "false") + ",";
  json += "\"boardId\":\"" + otaService.boardId() + "\",";
  json += "\"firmware\":\"" + String(APP_VERSION) + "\",";
  json += "\"otaConfigured\":" + String(OTA_MANIFEST_URL[0] != '\0' ? "true" : "false") + ",";
  json += "\"otaState\":\"" + String(otaService.stateName()) + "\",";
  json += "\"otaRemoteVersion\":\"" + htmlEscape(String(otaService.remoteVersion())) + "\",";
  json += "\"otaProgress\":" + String(otaService.progress()) + ",";
  json += "\"otaUpdateAvailable\":" + String(otaService.updateAvailable() ? "true" : "false") + ",";
  json += "\"otaError\":\"" + htmlEscape(String(otaService.lastError())) + "\",";
  json += "\"time\":\"" + currentTimeText() + "\",";
  json += "\"lastEvent\":\"" + htmlEscape(lastEvent) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleCommand() {
  if (!server.hasArg("c")) {
    server.send(400, "text/plain", "Missing c parameter");
    return;
  }
  String command = server.arg("c");
  command.trim();
  lastCommand = command;
  String lower = command;
  lower.toLowerCase();

  // Keep one command implementation for Serial and HTTP.
  processSerialCommand(lower);
  server.send(200, "text/plain", "OK " + lastEvent);
}

void handleWiFiForm() {
  String html = pageHeader("Configure Wi-Fi");
  html += "<h1>Configure Wi-Fi</h1><p>Use a 2.4 GHz network. The XIAO ESP32-S3 does not connect to 5 GHz.</p>";
  html += "<form method='POST' action='/wifi'><label>SSID</label><input name='ssid' required value='" + htmlEscape(savedSsid) + "'><label>Password</label><input name='password' type='password'><button type='submit'>Save and reboot</button></form>";
  html += "<p>After saving, reconnect your phone/computer to the normal network and open the new IP shown in Serial Monitor.</p></body></html>";
  server.send(200, "text/html", html);
}

void handleWiFiSave() {
  if (!server.hasArg("ssid") || !server.hasArg("password")) {
    server.send(400, "text/plain", "SSID and password are required");
    return;
  }
  String ssid = server.arg("ssid");
  String pass = server.arg("password");
  ssid.trim();
  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID cannot be empty");
    return;
  }
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  server.send(200, "text/html", "<h1>Saved</h1><p>Rebooting. Reconnect to your normal Wi-Fi, then open the new IP shown in Serial Monitor.</p>");
  delay(700);
  ESP.restart();
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

String pageHeader(const String &title) {
  String html = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>" + title + "</title><style>body{font-family:system-ui;background:#10131a;color:#f3f5f7;max-width:900px;margin:0 auto;padding:20px}section{background:#1b2230;border-radius:14px;padding:16px;margin:14px 0}button,.button{display:inline-block;border:0;border-radius:9px;background:#3d82f6;color:white;padding:11px 14px;margin:4px;text-decoration:none;font-size:15px}button:active{transform:scale(.97)}.stop{background:#d63855}.isabella{background:#3478f6;border:1px solid #8db7ff}.warn{color:#ffd166}.muted{color:#aeb8c9}input{padding:11px;border-radius:8px;border:1px solid #76809a;background:#0f131c;color:white;margin:5px;width:95%;box-sizing:border-box}label{display:block;margin-top:10px}pre{white-space:pre-wrap;background:#0c0f15;padding:12px;border-radius:8px}.ota{padding:12px;border-radius:8px;text-transform:capitalize;font-weight:600}.ota.up_to_date{background:#164b35;color:#b9ffd7}.ota.update_available,.ota.downloading,.ota.installing,.ota.rebooting{background:#594313;color:#ffe9a6}.ota.error{background:#5b1c2c;color:#ffd0da}.ota.disabled{background:#303744;color:#cbd5e1}</style></head><body>";
  return html;
}

String htmlEscape(const String &value) {
  String out = value;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "&quot;");
  out.replace("'", "&#39;");
  return out;
}

// ------------------------------ Face/personality layer ------------------------------

String faceName() {
  switch (faceMode) {
    case FACE_IDLE: return "idle";
    case FACE_LISTENING: return "listening";
    case FACE_THINKING: return "thinking";
    case FACE_SPEAKING: return "speaking";
    case FACE_HAPPY: return "happy";
    case FACE_LAUGH: return "laugh";
    case FACE_CURIOUS: return "curious";
    case FACE_SAD: return "sad";
    case FACE_SURPRISED: return "surprised";
    case FACE_WORKING: return "working";
    case FACE_ERROR: return "error";
    case FACE_SLEEP: return "sleep";
  }
  return "unknown";
}

String personalityName() {
  switch (personality) {
    case PERSONALITY_CALM: return "calm";
    case PERSONALITY_ROCKY: return "rocky";
    case PERSONALITY_ENGINEER: return "engineer";
    case PERSONALITY_SPARTAN: return "spartan";
    case PERSONALITY_ISABELLA: return "isabella";
  }
  return "calm";
}

String networkModeName() {
  if (provisioningAP) return "setup_ap";
  if (WiFi.status() == WL_CONNECTED) return "wifi_connected";
  return "wifi_disconnected";
}

String motorDriverName() {
  return "DRV8833 direct PWM inputs";
}

String cameraStatusName() {
  if (cameraReady) return "ready_snapshot";
  return "not_ready";
}

String currentTimeText() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 20)) return "not synced";
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  return String(buffer);
}

void updateIsabellaMood() {
  if (faceMode != FACE_IDLE && faceMode != FACE_HAPPY && faceMode != FACE_CURIOUS && faceMode != FACE_LAUGH && faceMode != FACE_SURPRISED) return;
  if (millis() < nextIsabellaMoodAt) return;

  const FaceMode moods[] = {FACE_HAPPY, FACE_CURIOUS, FACE_LAUGH, FACE_SURPRISED, FACE_IDLE};
  isabellaMood = moods[random(0, 5)];
  if (isabellaMood == FACE_IDLE) {
    faceMode = FACE_IDLE;
    lastEvent = "Isabella mood: soft idle";
  } else {
    faceMode = isabellaMood;
    lastEvent = "Isabella mood: " + faceName();
  }
  nextIsabellaMoodAt = millis() + random(ROCKY_ISABELLA_MOOD_MIN_MS, ROCKY_ISABELLA_MOOD_MAX_MS);
}

void setOledInverted(bool enabled, const String &reason) {
  oledInverted = enabled;
  preferences.putBool("oledInv", oledInverted);
  display.invertDisplay(oledInverted);
  lastEvent = reason + String(": ") + (oledInverted ? "on" : "off");
  Serial.println(lastEvent);
  drawFace();
}

void onOtaUiEvent(const char *state, int progress) {
  otaUiState = String(state);
  otaUiProgress = progress;
  if (otaUiState == "checking" || otaUiState == "update_available" || otaUiState == "downloading" || otaUiState == "installing" || otaUiState == "rebooting") {
    drawOtaIndicator();
  }
}

void drawOtaIndicator() {
  display.invertDisplay(oledInverted);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(8, 8);
  display.print("Deskbot OTA");
  display.setCursor(8, 23);
  display.print(otaUiState);
  if (otaUiState == "downloading" || otaUiState == "installing") {
    display.print(" ");
    display.print(otaUiProgress);
    display.print("%");
  }
  display.drawRect(8, 40, 112, 10, SSD1306_WHITE);
  const int width = constrain((otaUiProgress * 108) / 100, 0, 108);
  if (width > 0) display.fillRect(10, 42, width, 6, SSD1306_WHITE);
  display.display();
}

void drawFace() {
  if (!display.width()) return;
  display.invertDisplay(oledInverted);
  display.clearDisplay();

  // The referenced desktop-pet OLED task keeps character identity separate
  // from state and hardware. Isabella and Spartan use those imported
  // character renderers; the other personalities keep the bring-up renderer.
  if (drawImportedCharacter()) {
    drawWiFiIndicator();
    display.display();
    return;
  }

  drawWiFiIndicator();

  if (faceMode == FACE_SLEEP) {
    display.setTextSize(2);
    display.setCursor(45, 26);
    display.print("- -");
    display.setTextSize(1);
    display.setCursor(43, 52);
    display.print("sleep");
    display.display();
    return;
  }

  int eyeY = 20;
  int eyeH = 20;
  bool narrow = faceMode == FACE_SAD || faceMode == FACE_WORKING || personality == PERSONALITY_SPARTAN;
  bool round = personality == PERSONALITY_ROCKY || personality == PERSONALITY_ISABELLA || faceMode == FACE_HAPPY || faceMode == FACE_LAUGH || faceMode == FACE_SURPRISED;
  bool lookingLeft = panAngle < 65;
  bool lookingRight = panAngle > 115;

  // Thinking and working are deliberately different visual states.
  if (faceMode == FACE_THINKING) {
    eyeH = 17;
    lookingRight = ((faceFrame / 8) % 2) == 0;
    lookingLeft = !lookingRight;
  }
  if (faceMode == FACE_WORKING) {
    eyeH = 14;
    lookingLeft = ((faceFrame / 5) % 2) == 0;
    lookingRight = !lookingLeft;
  }
  if (faceMode == FACE_CURIOUS) eyeH = 24;
  if (faceMode == FACE_SURPRISED) eyeH = 26;

  drawEyes(eyeY, eyeH, narrow, round, lookingLeft, lookingRight);
  drawPersonalityAccent();
  drawMouth();

  display.setTextSize(1);
  display.setCursor(2, 56);
  if (faceMode == FACE_LISTENING) display.print("listening");
  else if (faceMode == FACE_THINKING) display.print("thinking...");
  else if (faceMode == FACE_WORKING) display.print("executing...");
  else if (faceMode == FACE_SPEAKING) display.print("speaking");
  else if (faceMode == FACE_ERROR) display.print("check system");
  else if (personality == PERSONALITY_ISABELLA) display.print("isabella");
  else display.print(personalityName());
  display.display();
}

void drawEyes(int eyeY, int eyeH, bool narrow, bool round, bool lookingLeft, bool lookingRight) {
  int leftX = 19;
  int rightX = 75;
  int eyeW = 34;
  int actualH = narrow ? 13 : eyeH;
  if (personality == PERSONALITY_CALM) actualH = min(actualH, 13);
  if (personality == PERSONALITY_ROCKY) actualH = min(actualH, 18);
  if (personality == PERSONALITY_ISABELLA) actualH = max(actualH, 23);
  int y = eyeY + (eyeH - actualH) / 2;

  if (personality == PERSONALITY_CALM) {
    display.fillRoundRect(leftX, y, eyeW, actualH, 2, SSD1306_WHITE);
    display.fillRoundRect(rightX, y, eyeW, actualH, 2, SSD1306_WHITE);
  } else if (personality == PERSONALITY_ROCKY) {
    display.fillRect(leftX, y, eyeW, actualH, SSD1306_WHITE);
    display.fillRect(rightX, y, eyeW, actualH, SSD1306_WHITE);
  } else if (personality == PERSONALITY_ISABELLA) {
    display.fillRoundRect(leftX, y, eyeW, actualH, 10, SSD1306_WHITE);
    display.fillRoundRect(rightX, y, eyeW, actualH, 10, SSD1306_WHITE);
  } else if (round) {
    display.fillRoundRect(leftX, y, eyeW, actualH, 6, SSD1306_WHITE);
    display.fillRoundRect(rightX, y, eyeW, actualH, 6, SSD1306_WHITE);
  } else {
    display.fillRoundRect(leftX, y, eyeW, actualH, 3, SSD1306_WHITE);
    display.fillRoundRect(rightX, y, eyeW, actualH, 3, SSD1306_WHITE);
  }

  int pupilShift = lookingLeft ? -7 : (lookingRight ? 7 : 0);
  if (faceMode == FACE_LISTENING) pupilShift = ((faceFrame / 5) % 2 == 0) ? -2 : 2;
  if (faceMode == FACE_SPEAKING) pupilShift = ((faceFrame / 4) % 2 == 0) ? -1 : 1;
  if (faceMode == FACE_WORKING) pupilShift = ((faceFrame / 3) % 2 == 0) ? -5 : 5;
  int pupilRadius = personality == PERSONALITY_ISABELLA ? 4 : (personality == PERSONALITY_ROCKY ? 2 : 3);
  display.fillCircle(leftX + eyeW / 2 + pupilShift, y + actualH / 2, pupilRadius, SSD1306_BLACK);
  display.fillCircle(rightX + eyeW / 2 + pupilShift, y + actualH / 2, pupilRadius, SSD1306_BLACK);
  if (personality == PERSONALITY_ISABELLA) {
    display.drawPixel(leftX + eyeW / 2 + pupilShift - 1, y + actualH / 2 - 1, SSD1306_WHITE);
    display.drawPixel(rightX + eyeW / 2 + pupilShift - 1, y + actualH / 2 - 1, SSD1306_WHITE);
  }

  if ((faceFrame % (personality == PERSONALITY_ISABELLA ? 47 : 83)) == 0 && (faceMode == FACE_IDLE || personality == PERSONALITY_ISABELLA)) {
    display.fillRect(leftX, eyeY + eyeH / 2 - 1, eyeW, 3, SSD1306_BLACK);
    display.fillRect(rightX, eyeY + eyeH / 2 - 1, eyeW, 3, SSD1306_BLACK);
  }
}

void drawPersonalityAccent() {
  if (personality == PERSONALITY_CALM) {
    display.drawLine(21, 15, 45, 13, SSD1306_WHITE);
    display.drawLine(83, 13, 107, 15, SSD1306_WHITE);
    display.drawPixel(10, 11, SSD1306_WHITE);
  } else if (personality == PERSONALITY_ROCKY) {
    display.drawLine(19, 14, 47, 10, SSD1306_WHITE);
    display.drawLine(81, 10, 109, 14, SSD1306_WHITE);
    display.drawCircle(8, 9, 3, SSD1306_WHITE);
    display.drawLine(8, 6, 8, 2, SSD1306_WHITE);
  } else if (personality == PERSONALITY_ENGINEER) {
    display.drawRect(14, 17, 43, 27, SSD1306_WHITE);
    display.drawRect(71, 17, 43, 27, SSD1306_WHITE);
    display.drawLine(57, 28, 71, 28, SSD1306_WHITE);
    display.drawLine(47, 11, 51, 6, SSD1306_WHITE);
    display.drawLine(51, 8, 57, 5, SSD1306_WHITE);
  } else if (personality == PERSONALITY_SPARTAN) {
    display.drawLine(47, 14, 64, 7, SSD1306_WHITE);
    display.drawLine(64, 7, 81, 14, SSD1306_WHITE);
    display.drawLine(16, 42, 29, 47, SSD1306_WHITE);
    display.drawLine(112, 42, 99, 47, SSD1306_WHITE);
  } else if (personality == PERSONALITY_ISABELLA) {
    // Soft animated Isabella cue: bow, sparkles, and a gentle center highlight.
    display.drawLine(6, 14, 6, 6, SSD1306_WHITE);
    display.drawLine(2, 10, 10, 10, SSD1306_WHITE);
    display.drawLine(116, 14, 116, 6, SSD1306_WHITE);
    display.drawLine(112, 10, 120, 10, SSD1306_WHITE);
    display.drawCircle(64, 11, 3, SSD1306_WHITE);
    display.drawLine(61, 11, 57, 7, SSD1306_WHITE);
    display.drawLine(67, 11, 71, 7, SSD1306_WHITE);
  }
}

void drawMouth() {
  int y = 48;
  bool animate = (faceMode == FACE_SPEAKING || faceMode == FACE_LAUGH);
  if (faceMode == FACE_SAD || faceMode == FACE_ERROR) {
    display.drawLine(54, y + 4, 64, y, SSD1306_WHITE);
    display.drawLine(64, y, 74, y + 4, SSD1306_WHITE);
    return;
  }
  if (faceMode == FACE_SURPRISED) {
    display.drawCircle(64, y + 2, 6, SSD1306_WHITE);
    return;
  }
  if (faceMode == FACE_HAPPY || faceMode == FACE_LAUGH) {
    display.drawLine(51, y, 58, y + 5, SSD1306_WHITE);
    display.drawLine(58, y + 5, 70, y + 5, SSD1306_WHITE);
    display.drawLine(70, y + 5, 77, y, SSD1306_WHITE);
    if (faceMode == FACE_LAUGH) display.drawLine(57, y + 8, 71, y + 8, SSD1306_WHITE);
    return;
  }
  if (faceMode == FACE_THINKING) {
    display.fillCircle(57, y + 3, 2, SSD1306_WHITE);
    display.fillCircle(64, y + 3, 2, SSD1306_WHITE);
    display.fillCircle(71, y + 3, 2, SSD1306_WHITE);
    return;
  }
  if (faceMode == FACE_WORKING) {
    display.drawLine(54, y + 3, 74, y + 3, SSD1306_WHITE);
    display.drawLine(58, y + 6, 70, y + 6, SSD1306_WHITE);
    return;
  }
  if (animate && ((faceFrame / 3) % 3 == 0)) {
    display.fillRoundRect(54, y - 2, 20, 11, 4, SSD1306_WHITE);
    display.drawLine(58, y + 3, 70, y + 3, SSD1306_BLACK);
  } else if (faceMode == FACE_LISTENING) {
    display.drawCircle(64, y + 2, 5, SSD1306_WHITE);
  } else if (faceMode == FACE_CURIOUS) {
    display.drawLine(55, y + 3, 73, y + 3, SSD1306_WHITE);
  } else if (personality == PERSONALITY_ISABELLA && ((faceFrame / 4) % 2 == 0)) {
    display.drawLine(51, y, 58, y + 5, SSD1306_WHITE);
    display.drawLine(58, y + 5, 70, y + 5, SSD1306_WHITE);
    display.drawLine(70, y + 5, 77, y, SSD1306_WHITE);
  } else if (personality == PERSONALITY_CALM) {
    display.drawLine(57, y + 3, 71, y + 3, SSD1306_WHITE);
  } else if (personality == PERSONALITY_ROCKY) {
    display.drawLine(53, y + 1, 59, y + 4, SSD1306_WHITE);
    display.drawLine(59, y + 4, 69, y + 4, SSD1306_WHITE);
    display.drawLine(69, y + 4, 75, y + 1, SSD1306_WHITE);
  } else {
    display.drawLine(55, y + 2, 73, y + 2, SSD1306_WHITE);
  }
}

void drawWiFiIndicator() {
  int x = 110;
  int y = 2;
  if (provisioningAP) {
    display.drawRect(x, y, 12, 9, SSD1306_WHITE);
    display.drawPixel(x + 3, y + 4, SSD1306_WHITE);
    display.drawPixel(x + 8, y + 4, SSD1306_WHITE);
    return;
  }
  if (WiFi.status() == WL_CONNECTED) {
    display.fillCircle(x + 6, y + 8, 2, SSD1306_WHITE);
    display.drawLine(x + 2, y + 5, x + 4, y + 3, SSD1306_WHITE);
    display.drawLine(x + 8, y + 3, x + 10, y + 5, SSD1306_WHITE);
    display.drawLine(x, y + 2, x + 3, y, SSD1306_WHITE);
    display.drawLine(x + 9, y, x + 12, y + 2, SSD1306_WHITE);
  } else {
    display.drawLine(x, y, x + 12, y + 10, SSD1306_WHITE);
    display.drawLine(x + 12, y, x, y + 10, SSD1306_WHITE);
  }
}

// End of modular XIAO bring-up firmware.
// Voice, speaker playback, wake-word, and LLM modules remain separate next
// stages; camera, microphone level monitoring, dashboard, and OTA are present.
