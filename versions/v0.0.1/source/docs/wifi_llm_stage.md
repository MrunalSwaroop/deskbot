# Stage 3: UNO R4 WiFi plus a local LLM command bridge

The UNO R4 WiFi can now run the face, one-axis pan servo, and a small local HTTP server. A laptop on the same Wi-Fi network can ask an LLM to translate natural language into one safe face or pan command.

The LLM key stays on the laptop. It is never stored in the Arduino sketch and never sent to the UNO.

## Files

Keep these files together in one Arduino sketch folder:

```text
uno_wifi_face_server/
├── uno_wifi_face_server.ino
├── eyes.h
└── arduino_secrets.h
```

Use `arduino_secrets.h.example` as the template for `arduino_secrets.h`.

The local LLM bridge is separate:

```text
llm_bridge/
└── robot_llm_bridge.py
```

## Part A: configure the UNO Wi-Fi sketch

### Install the board package

In Arduino IDE 2:

1. Open **Tools → Board → Boards Manager**.
2. Search for **UNO R4 WiFi**.
3. Install the **Arduino UNO R4 Boards** package.
4. Select **Arduino UNO R4 WiFi**.
5. Connect the board through USB-C and select its port.

The UNO R4 WiFi includes the `WiFiS3` library through its board package. The official Arduino examples use `#include <WiFiS3.h>` and a `WiFiServer` for local web-server tests.[1] [2]

### Add credentials

Copy `arduino_secrets.h.example` to a new file named exactly:

```text
arduino_secrets.h
```

Edit it:

```cpp
#define SECRET_SSID "your Wi-Fi name"
#define SECRET_PASS "your Wi-Fi password"
```

Do not share `arduino_secrets.h` publicly.

### Hardware for this stage

Keep the OLED wiring that already works. The optional pan servo uses:

| Function | UNO pin |
|---|---:|
| Pan servo signal | D3 |
| OLED SDA | SDA/A4 |
| OLED SCL | SCL/A5 |

No tilt servo and no motor driver are required for this stage.

### Upload and find the IP address

Upload `uno_wifi_face_server.ino`. Open Serial Monitor at **115200 baud**.

You should see something like:

```text
Connecting to Wi-Fi: your-network
...
WIFI OK IP: 192.168.1.123
HTTP: GET http://ROBOT_IP/health or /cmd/happy
```

Record the IP address. The board and laptop must be connected to the same 2.4 GHz Wi-Fi network. Some guest networks prevent devices from talking to one another; use a normal private network.

If Wi-Fi fails, the sketch still continues in USB serial mode. Check the SSID/password, Wi-Fi band, and whether the UNO Wi-Fi module firmware needs updating.

## Part B: test the robot HTTP API without an LLM

Replace `192.168.1.123` with the actual IP printed by your board.

From a browser, open:

```text
http://192.168.1.123/health
```

You should see a response such as:

```text
OK IP=192.168.1.123 PAN=90
```

Then test the face:

```text
http://192.168.1.123/cmd/happy
http://192.168.1.123/cmd/listen
http://192.168.1.123/cmd/speak
http://192.168.1.123/cmd/sleep
http://192.168.1.123/cmd/surprise
```

Test the pan servo, if connected:

```text
http://192.168.1.123/cmd/look%20left
http://192.168.1.123/cmd/look%20center
http://192.168.1.123/cmd/look%20right
http://192.168.1.123/cmd/pan%2090
```

The HTTP API supports only these commands in this stage:

```text
idle
happy
listen
speak
sleep
surprise
sad
curious
worried
look left
look right
look center
```

The firmware has no motor endpoint yet. This is intentional. Keep the first Wi-Fi and LLM tests limited to face and pan behavior.

## Part C: run the local LLM bridge

The bridge runs on your laptop, not on the Arduino.

Install its Python dependencies:

```bash
python3 -m pip install flask openai requests
```

Set the robot IP:

```bash
export ROBOT_URL="http://192.168.1.123"
```

Set your LLM credentials. For OpenAI:

```bash
export OPENAI_API_KEY="your-api-key"
export LLM_MODEL="gpt-4o-mini"
```

For another OpenAI-compatible provider, set its base URL and model according to that provider's instructions. Do not put the API key in the Arduino sketch.

Start the bridge:

```bash
cd /path/to/rocky_walle_poc/llm_bridge
python3 robot_llm_bridge.py
```

It listens only on `127.0.0.1:5050` by default.

Test it directly:

```bash
curl http://127.0.0.1:5050/health
```

Ask the LLM for a face command:

```bash
curl -X POST http://127.0.0.1:5050/chat \\
  -H 'Content-Type: application/json' \\
  -d '{"text":"Please greet me warmly"}'
```

A successful response looks like:

```json
{
  "ok": true,
  "input": "Please greet me warmly",
  "command": "happy",
  "robot": "OK command=happy"
}
```

Try:

```bash
curl -X POST http://127.0.0.1:5050/chat -H 'Content-Type: application/json' -d '{"text":"Listen carefully"}'
curl -X POST http://127.0.0.1:5050/chat -H 'Content-Type: application/json' -d '{"text":"Look to the left"}'
curl -X POST http://127.0.0.1:5050/chat -H 'Content-Type: application/json' -d '{"text":"Speak now"}'
curl -X POST http://127.0.0.1:5050/chat -H 'Content-Type: application/json' -d '{"text":"Go back to the center"}'
```

The LLM is constrained to a small allow-list. If the model produces anything unexpected, the bridge falls back to `idle`. This is safer than letting an LLM generate arbitrary URLs or motor commands.

## What this stage can and cannot do

The UNO R4 WiFi can now demonstrate:

- Wi-Fi connection and IP discovery.
- Browser-based face control.
- Browser-based one-axis pan control.
- Natural-language-to-face command translation through a laptop LLM bridge.
- A stable path for later adding a microphone, speaker, camera, and richer AI behavior.

It cannot yet do the final Rocky experience by itself. The UNO does not provide the XIAO Sense's integrated camera and digital microphone, and this stage has no speaker or text-to-speech output. The first LLM POC is therefore visual: the model chooses a face or pan behavior, and the OLED displays it.

When the XIAO ESP32S3 Sense arrives, it can replace the laptop-side or generic ESP32-side interaction layer. Keep the command vocabulary unchanged so the UNO actuator code remains reusable.

## Security and safety

The UNO HTTP server is plain HTTP and has no login. Keep it on your private LAN. Do not port-forward it to the internet.

The laptop bridge is bound to localhost. Do not change it to `0.0.0.0` until you intentionally add authentication.

Do not put an LLM API key in `uno_wifi_face_server.ino`, `arduino_secrets.h`, or any public repository. The Wi-Fi password belongs in `arduino_secrets.h`, which should remain private.

Do not let this stage control motors. If motor control is added later, use fixed-duration commands, a hardware stop switch, and an allow-list that excludes unrestricted speed or duration.

## References

[1]: https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples "Arduino UNO R4 WiFi network examples"
[2]: https://docs.arduino.cc/tutorials/uno-r4-wifi/r4-wifi-getting-started "Arduino UNO R4 WiFi getting started guide"
[3]: https://docs.arduino.cc/hardware/uno-r4-wifi "Arduino UNO R4 WiFi official hardware page"
