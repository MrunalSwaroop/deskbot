#include <WiFi.h>
#include <WebServer.h>

// Rocky-Wall-E POC: generic ESP32 Wi-Fi command gateway
//
// This board does not need to know anything about OLED rendering or motor
// polarity. It only receives commands over USB/Wi-Fi and forwards them to the
// UNO R4 over UART. The future XIAO ESP32S3 Sense can replace this gateway.

const char *WIFI_SSID = "REPLACE_WITH_WIFI_NAME";
const char *WIFI_PASSWORD = "REPLACE_WITH_WIFI_PASSWORD";

// Change these two pins to match your ESP32 board. GPIO16/17 are common on
// ESP32-WROOM DevKit boards, but never assume this without checking the pinout.
const int ESP32_RX_FROM_UNO = 16;
const int ESP32_TX_TO_UNO = 17;

HardwareSerial UnoSerial(2);
WebServer server(80);
String usbLine;

void setup() {
  Serial.begin(115200);
  UnoSerial.begin(115200, SERIAL_8N1, ESP32_RX_FROM_UNO, ESP32_TX_TO_UNO);
  usbLine.reserve(96);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 20000) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("READY http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WARN Wi-Fi not connected; USB forwarding still works");
  }

  server.on("/health", HTTP_GET, []() {
    server.send(200, "text/plain", "ROCKY_WALLE_GATEWAY_OK");
  });

  server.on("/cmd", HTTP_GET, []() {
    if (!server.hasArg("x")) {
      server.send(400, "text/plain", "Missing query parameter x");
      return;
    }
    String command = server.arg("x");
    forward(command);
    server.send(200, "text/plain", "OK forwarded: " + command);
  });

  server.begin();
}

void loop() {
  server.handleClient();
  readUsbCommands();

  while (UnoSerial.available()) {
    Serial.write(UnoSerial.read());
  }
}

void readUsbCommands() {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (usbLine.length() > 0) {
        forward(usbLine);
        usbLine = "";
      }
    } else if (usbLine.length() < 95) {
      usbLine += c;
    }
  }
}

void forward(String command) {
  command.trim();
  if (command.length() == 0) return;
  UnoSerial.println(command);
  Serial.println("TX> " + command);
}
