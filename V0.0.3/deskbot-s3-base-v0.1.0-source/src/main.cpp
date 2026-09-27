#include <Arduino.h>
#include <WebServer.h>

#include "DeskBotConfig.h"
#include "HardwareManager.h"
#include "NetworkManager.h"

namespace {

WebServer server(80);
NetworkManager network;
HardwareManager hardware;
uint32_t restartAtMs = 0;

String jsonEscape(const String &value) {
  String result;
  result.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char character = value[i];
    if (character == '\\' || character == '"') result += '\\';
    if (character == '\n') result += "\\n";
    else result += character;
  }
  return result;
}

String deviceStatusJson() {
  const NetworkStatus currentNetwork = network.status();
  const char *mode = currentNetwork.mode == NetworkMode::Station ? "station" : "setup_ap";
  return "{\"firmware\":\"" + String(DeskBotConfig::FIRMWARE_NAME) +
         "\",\"version\":\"" + String(DeskBotConfig::FIRMWARE_VERSION) +
         "\",\"board\":\"" + String(DeskBotConfig::BOARD_NAME) +
         "\",\"network\":{\"mode\":\"" + String(mode) +
         "\",\"ssid\":\"" + jsonEscape(currentNetwork.ssid) +
         "\",\"ip\":\"" + currentNetwork.ip +
         "\",\"rssi\":" + String(currentNetwork.rssi) +
         "},\"motors_armed\":" + String(hardware.motorArmed() ? "true" : "false") +
         ",\"camera_ready\":" + String(hardware.cameraReady() ? "true" : "false") +
         ",\"camera_status\":\"" + jsonEscape(hardware.cameraStatus()) + "\"}";
}

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>DeskBot-S3 Base</title><style>
:root{color-scheme:dark;--ink:#eaf2ff;--muted:#99a9c2;--panel:#121c2d;--line:#26364f;--cyan:#32d8e5;--amber:#f6ba4e;--danger:#ff6676}*{box-sizing:border-box}body{margin:0;background:#090f1a;color:var(--ink);font:15px system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;line-height:1.45}.wrap{max-width:980px;margin:auto;padding:26px 18px 60px}header{display:flex;gap:16px;align-items:center;border-bottom:1px solid var(--line);padding-bottom:22px;margin-bottom:22px}.mark{width:48px;height:48px;display:grid;place-items:center;border:1px solid #29636e;border-radius:14px;color:var(--cyan);font-size:24px;background:#0e2630}h1{font-size:24px;margin:0}h2{font-size:16px;margin:0 0 10px}.sub{color:var(--muted);font-size:13px;margin:3px 0 0}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:16px}.wide{grid-column:1/-1}.card{background:linear-gradient(145deg,#152238,#0f1828);border:1px solid var(--line);border-radius:14px;padding:18px}.pill{display:inline-block;font:12px ui-monospace,monospace;color:var(--cyan);border:1px solid #276773;background:#0c2730;border-radius:999px;padding:4px 8px}.warn{border-color:#6f5221;background:linear-gradient(145deg,#2d2413,#19170f)}.warn strong{color:var(--amber)}label{display:block;font-size:12px;color:var(--muted);margin:10px 0 5px}input{width:100%;background:#0a111e;border:1px solid #33445d;border-radius:8px;color:var(--ink);padding:10px}button{border:1px solid #237f8b;background:#0d3540;color:#d9fbff;border-radius:8px;padding:9px 12px;margin:8px 6px 0 0;font-weight:650;cursor:pointer}button:hover{background:#13505d}button.danger{border-color:#9a3d4b;background:#381620;color:#ffdce0}button:disabled{opacity:.45;cursor:not-allowed}.kv{display:grid;grid-template-columns:130px 1fr;gap:6px;font-size:13px}.key{color:var(--muted)}#i2c{font-family:ui-monospace,monospace;color:var(--cyan);min-height:24px}#camera{width:100%;border-radius:9px;background:#05080e;margin-top:10px;min-height:4px}.footer{margin-top:18px;color:var(--muted);font-size:12px}.ok{color:#4ce09a}.bad{color:var(--danger)}@media(max-width:700px){.grid{grid-template-columns:1fr}.wide{grid-column:auto}.kv{grid-template-columns:105px 1fr}}
</style></head><body><main class="wrap"><header><div class="mark">◉</div><div><h1>DeskBot-S3 Base <span class="pill">v0.1.0</span></h1><p class="sub">Standalone, modular ESP32-S3 foundation • local Wi-Fi setup • safe hardware diagnostics</p></div></header>
<section class="card warn"><strong>Electrical safety:</strong> Disconnect the robot battery while using USB. Motors are disabled on boot and auto-stop 500 ms after the final command. Use a separate 5 V regulator for motors/servos; share ground with the XIAO but never power motors from its 3.3 V rail.</section>
<section class="grid" style="margin-top:16px"><div class="card"><h2>Device & network</h2><div class="kv"><span class="key">State</span><span id="netMode">Loading…</span><span class="key">Network</span><span id="netSsid">—</span><span class="key">Address</span><span id="netIp">—</span><span class="key">Camera</span><span id="cameraState">—</span></div><p class="footer">If Wi-Fi setup is needed, join <b>DeskBot-S3-Setup</b> (password <b>deskbot-s3</b>) then open <b>http://192.168.4.1</b>.</p></div>
<div class="card"><h2>Change Wi-Fi / move locations</h2><form id="wifiForm"><label>Wi-Fi network name (SSID)</label><input id="ssid" maxlength="32" autocomplete="off" required><label>Wi-Fi password</label><input id="password" type="password" maxlength="63"><button type="submit">Save & restart</button></form><button class="danger" onclick="forgetWifi()">Forget saved Wi-Fi</button><p class="footer" id="wifiResult">Credentials are stored only in the ESP32-S3's encrypted NVS area when flash encryption is enabled later; this base stores them in local NVS for development.</p></div>
<div class="card"><h2>I²C diagnostic</h2><p class="sub">Default safe XIAO wiring: SDA = D4 / GPIO 5, SCL = D5 / GPIO 6. Scan before enabling a sensor module.</p><button onclick="scanI2c()">Scan I²C bus</button><div id="i2c">No scan performed.</div></div>
<div class="card"><h2>Motor driver safety test</h2><p class="sub">Default pins: Left IN1/IN2 = D0/D1, Right IN1/IN2 = D2/D3. Confirm your driver logic-voltage compatibility before arming.</p><button id="armButton" onclick="toggleArm()">Arm motor outputs</button><div id="motorControls" style="display:none"><button onclick="drive(90,90)">Forward</button><button onclick="drive(-90,-90)">Reverse</button><button onclick="drive(-90,90)">Turn left</button><button onclick="drive(90,-90)">Turn right</button><button class="danger" onclick="drive(0,0)">STOP</button></div></div>
<div class="card wide"><h2>XIAO ESP32-S3 Sense camera (optional)</h2><p class="sub">The firmware attempts the official XIAO Sense camera map. A plain XIAO ESP32-S3 continues without this module.</p><button onclick="capture()">Capture a test frame</button><span id="captureResult" class="sub"></span><img id="camera" alt="Camera preview appears here"></div></section>
<p class="footer">Modular base release. Add VL53L5CX, ISM330DHCX, MMC5983MA, motor driver, and camera one at a time; test each device with the I²C scanner and motor safety controls before enabling autonomous behavior.</p></main><script>
async function api(path,method='GET'){const r=await fetch(path,{method});if(!r.ok)throw new Error(await r.text());return r.json()}
async function refresh(){try{const s=await api('/api/status');netMode.textContent=s.network.mode==='station'?'Connected to local Wi-Fi':'Setup access point active';netSsid.textContent=s.network.ssid;netIp.textContent=s.network.ip;cameraState.textContent=s.camera_ready?'Ready':'Unavailable / optional';cameraState.className=s.camera_ready?'ok':'bad';armButton.textContent=s.motors_armed?'Disarm motor outputs':'Arm motor outputs';motorControls.style.display=s.motors_armed?'block':'none'}catch(e){netMode.textContent='Connection error'}}
async function scanI2c(){i2c.textContent='Scanning…';try{const d=await api('/api/i2c/scan');i2c.textContent=d.devices.length?'Found: '+d.devices.join(', '):'No I²C addresses found. Check voltage, common ground, SDA/SCL, and pull-ups.'}catch(e){i2c.textContent='Scan error: '+e.message}}
async function toggleArm(){try{await api('/api/motors/arm?enabled=1','POST');await refresh()}catch(e){alert(e.message)}}
async function drive(left,right){try{await api('/api/motors/run?left='+left+'&right='+right,'POST')}catch(e){alert('Motor command rejected: '+e.message)}}
async function forgetWifi(){if(!confirm('Forget saved Wi-Fi and restart into setup mode?'))return;await api('/api/wifi/reset','POST');wifiResult.textContent='Resetting…'}
async function capture(){captureResult.textContent='Capturing…';const r=await fetch('/camera/capture?'+Date.now());if(!r.ok){captureResult.textContent='Camera not available on this board.';return}camera.src=URL.createObjectURL(await r.blob());captureResult.textContent='Frame captured.'}
wifiForm.onsubmit=async(e)=>{e.preventDefault();const ssid=encodeURIComponent(document.getElementById('ssid').value),password=encodeURIComponent(document.getElementById('password').value);try{await api('/api/wifi?ssid='+ssid+'&password='+password,'POST');wifiResult.textContent='Saved. The board will restart and join the selected network.'}catch(err){wifiResult.textContent='Could not save: '+err.message}};refresh();setInterval(refresh,5000);
</script></body></html>
)HTML";

void scheduleRestart() {
  restartAtMs = millis() + 1200;
}

void sendJson(int statusCode, const String &payload) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(statusCode, "application/json", payload);
}

void registerRoutes() {
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/api/status", HTTP_GET, []() {
    sendJson(200, deviceStatusJson());
  });

  server.on("/api/i2c/scan", HTTP_GET, []() {
    sendJson(200, hardware.scanI2cJson());
  });

  server.on("/api/wifi", HTTP_POST, []() {
    const String ssid = server.arg("ssid");
    const String password = server.arg("password");
    if (!network.saveCredentials(ssid, password)) {
      sendJson(400, "{\"error\":\"SSID must be 1-32 chars and password no more than 63 chars\"}");
      return;
    }
    sendJson(200, "{\"ok\":true,\"message\":\"Credentials saved; restarting\"}");
    scheduleRestart();
  });

  server.on("/api/wifi/reset", HTTP_POST, []() {
    network.clearCredentials();
    sendJson(200, "{\"ok\":true,\"message\":\"Saved credentials cleared; restarting setup portal\"}");
    scheduleRestart();
  });

  server.on("/api/motors/arm", HTTP_POST, []() {
    const bool shouldArm = server.arg("enabled") == "1" || server.arg("enabled") == "true";
    hardware.setMotorArmed(shouldArm);
    sendJson(200, "{\"armed\":" + String(hardware.motorArmed() ? "true" : "false") + "}");
  });

  server.on("/api/motors/run", HTTP_POST, []() {
    if (!hardware.motorArmed()) {
      sendJson(403, "{\"error\":\"Motor outputs are not armed\"}");
      return;
    }
    const int left = server.arg("left").toInt();
    const int right = server.arg("right").toInt();
    hardware.setMotors(left, right);
    sendJson(200, "{\"left\":" + String(left) + ",\"right\":" + String(right) + "}");
  });

  server.on("/camera/capture", HTTP_GET, []() {
    hardware.sendCameraSnapshot(server);
  });

  server.onNotFound([]() {
    if (network.isSetupMode()) {
      server.sendHeader("Location", "http://192.168.4.1/", true);
      server.send(302, "text/plain", "Captive portal");
      return;
    }
    sendJson(404, "{\"error\":\"Not found\"}");
  });
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(350);
  Serial.println();
  Serial.println("====================================================");
  Serial.printf("%s v%s\n", DeskBotConfig::FIRMWARE_NAME, DeskBotConfig::FIRMWARE_VERSION);
  Serial.printf("Target: %s\n", DeskBotConfig::BOARD_NAME);
  Serial.println("Motors are OFF by default. Configure and test one module at a time.");
  Serial.println("====================================================");

  hardware.begin();
  network.begin();
  registerRoutes();
  server.begin();
  Serial.println("[web] Local administration server ready on port 80.");
}

void loop() {
  server.handleClient();
  network.loop();
  hardware.loop();

  if (restartAtMs != 0 && millis() >= restartAtMs) {
    ESP.restart();
  }
}
