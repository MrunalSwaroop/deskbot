#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include "eyes.h"

// Rocky-Wall-E POC: UNO R4 WiFi controller
//
// This controller deliberately uses a small line-based command protocol so the
// future XIAO ESP32S3 Sense can replace the current command source without
// changing the face/motion firmware.
//
// USB commands are received on Serial. A future Wi-Fi ESP32 gateway can send
// the same commands to Serial1 (UNO pins D0/D1).

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Servo panServo;
Servo tiltServo;

// Servo pins.
const uint8_t PAN_SERVO_PIN = 3;
const uint8_t TILT_SERVO_PIN = 4;

// L298N motor driver pins. Do not power motors from the UNO 5V pin.
const uint8_t ENA_PIN = 5;
const uint8_t IN1_PIN = 6;
const uint8_t IN2_PIN = 7;
const uint8_t ENB_PIN = 10;
const uint8_t IN3_PIN = 11;
const uint8_t IN4_PIN = 12;

// Current state.
int eyeMood = 1;       // 0..5 entries from eyes.h
int eyePosition = 16;  // 0 = left, 16 = centre, 32 = right
int eyeJitter = 0;
int panAngle = 90;
int tiltAngle = 90;
int driveSpeed = 150;

unsigned long nextEyeFrameAt = 0;
unsigned long nextBlinkAt = 0;
bool blinkNow = false;
String inputLine;

void setup() {
  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  stopMotors();

  panServo.attach(PAN_SERVO_PIN);
  tiltServo.attach(TILT_SERVO_PIN);
  panServo.write(panAngle);
  tiltServo.write(tiltAngle);

  Serial.begin(115200);
  Serial1.begin(115200);  // UNO R4 D0/D1; use only with a protected 3.3 V UART.
  inputLine.reserve(96);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("ERR OLED init failed; check address and I2C wiring");
    while (true) {
      delay(1000);
    }
  }

  randomSeed(analogRead(A0));
  setFace("idle");
  Serial.println("OK ROCKY_WALLE_UNO_READY");
  Serial.println("Commands: HELP, EYE idle|listen|speak|happy|sleep|surprise|left|right, MOVE stop|forward|back|left|right, SPEED 0..255, SERVO pan|tilt 0..180");
}

void loop() {
  readCommandsFrom(Serial);
  readCommandsFrom(Serial1);
  updateFaceAnimation();
}

void readCommandsFrom(Stream &port) {
  while (port.available()) {
    char c = static_cast<char>(port.read());
    if (c == '\n' || c == '\r') {
      if (inputLine.length() > 0) {
        processCommand(inputLine, port);
        inputLine = "";
      }
    } else if (inputLine.length() < 95) {
      inputLine += c;
    }
  }
}

void reply(Stream &port, const char *message) {
  port.println(message);
  // Also mirror gateway results to USB when the command arrived through Serial1.
  if (&port != &Serial) {
    Serial.println(message);
  }
}

void processCommand(String command, Stream &port) {
  command.trim();
  command.toLowerCase();
  if (command.length() == 0) return;

  if (command == "help") {
    reply(port, "OK HELP: EYE idle|listen|speak|happy|sleep|surprise|left|right; MOVE stop|forward|back|left|right; SPEED n; SERVO pan|tilt n");
    return;
  }

  if (command.startsWith("eye ")) {
    setFace(command.substring(4));
    reply(port, "OK EYE");
    return;
  }

  if (command.startsWith("move ")) {
    drive(command.substring(5));
    reply(port, "OK MOVE");
    return;
  }

  if (command.startsWith("speed ")) {
    int value = constrain(command.substring(6).toInt(), 0, 255);
    driveSpeed = value;
    reply(port, "OK SPEED");
    return;
  }

  if (command.startsWith("servo ")) {
    handleServo(command.substring(6), port);
    return;
  }

  reply(port, "ERR UNKNOWN_COMMAND");
}

void handleServo(String payload, Stream &port) {
  payload.trim();
  int separator = payload.indexOf(' ');
  if (separator < 0) {
    reply(port, "ERR SERVO_SYNTAX");
    return;
  }

  String which = payload.substring(0, separator);
  int value = constrain(payload.substring(separator + 1).toInt(), 0, 180);
  if (which == "pan") {
    panAngle = value;
    panServo.write(panAngle);
    reply(port, "OK SERVO PAN");
  } else if (which == "tilt") {
    tiltAngle = value;
    tiltServo.write(tiltAngle);
    reply(port, "OK SERVO TILT");
  } else {
    reply(port, "ERR SERVO_NAME");
  }
}

void setFace(String face) {
  face.trim();
  blinkNow = false;

  if (face == "idle") {
    eyeMood = 1;
    eyePosition = 16;
  } else if (face == "listen") {
    eyeMood = 3;
    eyePosition = 16;
  } else if (face == "speak") {
    eyeMood = 4;
    eyePosition = 16;
  } else if (face == "happy") {
    eyeMood = 0;
    eyePosition = 16;
  } else if (face == "sleep") {
    eyeMood = 2;
    eyePosition = 16;
  } else if (face == "surprise") {
    eyeMood = 5;
    eyePosition = 16;
  } else if (face == "left") {
    eyeMood = 1;
    eyePosition = 0;
  } else if (face == "right") {
    eyeMood = 1;
    eyePosition = 32;
  } else {
    return;
  }

  nextEyeFrameAt = 0;
  drawEyes();
}

void updateFaceAnimation() {
  unsigned long now = millis();
  if (now >= nextBlinkAt) {
    blinkNow = true;
    nextBlinkAt = now + random(2500, 6500);
    drawEyes();
    return;
  }

  if (blinkNow && now >= nextEyeFrameAt) {
    blinkNow = false;
    nextEyeFrameAt = now + 80;
    drawEyes();
    return;
  }

  // Subtle eye motion makes the idle face feel alive without blocking motion.
  if (now >= nextEyeFrameAt && eyeMood == 1) {
    eyeJitter = random(-2, 3);
    nextEyeFrameAt = now + random(600, 1400);
    drawEyes();
  }
}

void drawEyes() {
  int x1 = eyePosition + eyeJitter;
  int x2 = 64 + eyePosition + eyeJitter;
  x1 = constrain(x1, 0, 96);
  x2 = constrain(x2, 32, 127);

  display.clearDisplay();
  if (blinkNow || eyeMood == 2) {
    display.drawBitmap(x1, 16, eye0, 32, 32, SSD1306_WHITE);
    display.drawBitmap(x2, 16, eye0, 32, 32, SSD1306_WHITE);
  } else {
    const unsigned char *leftEye = peyes[eyeMood][0][0];
    const unsigned char *rightEye = peyes[eyeMood][0][1];
    display.drawBitmap(x1, 8, leftEye, 32, 32, SSD1306_WHITE);
    display.drawBitmap(x2, 8, rightEye, 32, 32, SSD1306_WHITE);
  }
  display.display();
}

void drive(String direction) {
  direction.trim();
  if (direction == "stop") {
    stopMotors();
  } else if (direction == "forward") {
    motor(ENA_PIN, IN1_PIN, IN2_PIN, driveSpeed, true);
    motor(ENB_PIN, IN3_PIN, IN4_PIN, driveSpeed, true);
  } else if (direction == "back") {
    motor(ENA_PIN, IN1_PIN, IN2_PIN, driveSpeed, false);
    motor(ENB_PIN, IN3_PIN, IN4_PIN, driveSpeed, false);
  } else if (direction == "left") {
    motor(ENA_PIN, IN1_PIN, IN2_PIN, driveSpeed, false);
    motor(ENB_PIN, IN3_PIN, IN4_PIN, driveSpeed, true);
  } else if (direction == "right") {
    motor(ENA_PIN, IN1_PIN, IN2_PIN, driveSpeed, true);
    motor(ENB_PIN, IN3_PIN, IN4_PIN, driveSpeed, false);
  }
}

void motor(uint8_t enablePin, uint8_t inA, uint8_t inB, int pwm, bool forward) {
  analogWrite(enablePin, pwm);
  digitalWrite(inA, forward ? HIGH : LOW);
  digitalWrite(inB, forward ? LOW : HIGH);
}

void stopMotors() {
  analogWrite(ENA_PIN, 0);
  analogWrite(ENB_PIN, 0);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
}
