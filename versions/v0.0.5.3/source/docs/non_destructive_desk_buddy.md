# Non-destructive Rocky-Wall-E desk buddy

This version uses the **Arduino UNO R4 WiFi exactly as it is**. It does not reflash, erase, or replace the UNO R4’s onboard ESP32-S3 bridge firmware.

The uploaded sketch runs on the normal RA4M1 Arduino side and uses the existing `WiFiS3` library and bridge firmware for network access.

## Files

Keep these files together in one Arduino sketch folder:

```text
uno_r4_current_desk_buddy/
├── uno_r4_current_desk_buddy.ino
├── eyes.h
├── arduino_secrets.h
```

Copy `arduino_secrets.h.example` to `arduino_secrets.h` and fill in the private values.

## Hardware

Keep the OLED wiring that already works:

| Device | UNO R4 pin |
|---|---|
| OLED SDA | SDA/A4 |
| OLED SCL | SCL/A5 |
| OLED VCC | According to the OLED module marking |
| OLED GND | GND |
| Pan servo signal | D3 |
| L293D EN1 / left enable | D5 |
| L293D IN1 / left direction 1 | D6 |
| L293D IN2 / left direction 2 | D7 |
| L293D EN2 / right enable | D9 |
| L293D IN3 / right direction 1 | D10 |
| L293D IN4 / right direction 2 | D11 |

The two N90 motors connect to the L293D motor outputs. Use the L293D logic supply for its logic side and a separate motor supply appropriate for the N90 motors. Connect the motor-supply ground, L293D ground, and UNO GND together. Do not power the motors from an Arduino GPIO pin or directly from the UNO 5 V pin.

Use a suitable external 5 V supply for the servo when necessary. Connect that supply’s ground to UNO GND. Do not power a motor from an Arduino GPIO pin.

If using an L293D module with EN jumpers, remove the EN1/EN2 jumpers before connecting D5/D9 so the UNO can control speed with PWM. Lift the robot’s wheels before the first test. If one motor spins backward, swap that motor’s two output wires or reverse that side in `setMotor()`.

## Wi-Fi and direct LLM configuration

The sketch uses the same `WiFiS3` architecture as the standard UNO R4 WiFi examples. It does not program the onboard ESP32-S3 directly.

In `arduino_secrets.h`:

```cpp
#define SECRET_SSID "YOUR_WIFI_NAME"
#define SECRET_PASS "YOUR_WIFI_PASSWORD"
#define LLM_API_KEY "YOUR_LLM_API_KEY"
#define LLM_HOST "api.openai.com"
#define LLM_PATH "/v1/chat/completions"
#define LLM_MODEL "gpt-4o-mini"
```

If `LLM_API_KEY` is left empty, all face, dance, pan, and notification commands still work, but `ask ...` and `/input/...` show an error face.

Use a restricted or short-lived API key during the prototype. Do not commit the real secrets file to GitHub or include it in a shared ZIP.

## Upload

1. Install the **Arduino UNO R4 Boards** package in Arduino IDE.
2. Install **Adafruit GFX Library**, **Adafruit SSD1306**, and **Servo**.
3. Select **Arduino UNO R4 WiFi**.
4. Select the UNO USB port.
5. Upload `uno_r4_current_desk_buddy.ino`.
6. Open Serial Monitor at **115200 baud**.

### If Arduino reports “was not declared in this scope”

Use the corrected sketch from this package. The large dashboard sketch now includes explicit forward declarations for functions defined later in the file, including `showBootGreeting`, `updatePersonality`, `drawFace`, `drawEyebrows`, `drawCheeks`, and `drawMouth`. This avoids a limitation in Arduino IDE's automatic prototype generation.

Do not copy only part of the `.ino` file. Replace the entire sketch with the corrected file, keep `eyes.h` beside it, and compile again.

The Serial Monitor should show the Wi-Fi IP address and:

```text
Rocky desk buddy ready; existing UNO R4 WiFi architecture preserved.
```

The top-right OLED icon is a small Wi-Fi indicator: a connected symbol means the UNO has an active Wi-Fi link; a crossed symbol means it is offline. The Serial Monitor now prints the Wi-Fi firmware version, attempt number, status code, IP address, and RSSI.

The firmware now waits for DHCP after Wi-Fi association. A strong RSSI, such as `-39`, only proves that the radio can see the access point; it does not prove that the router has assigned an IP address. The dashboard is not started until the address is non-zero, so `0.0.0.0` will not be advertised as a usable URL.

Wait for:

```text
WIFI OK IP: 192.168.x.x
DASHBOARD: http://192.168.x.x
```

If DHCP still fails after the wait, the sketch prints a no-DHCP error and retries while keeping the face and Serial Monitor available.

If it does not connect, first type:

```text
status
```

Then check the following:

1. `SECRET_SSID` and `SECRET_PASS` are in a private `arduino_secrets.h`, not only in the `.example` file.
2. The network is 2.4 GHz. Many phone hotspots default to 5 GHz; switch the hotspot to 2.4 GHz compatibility mode.
3. The network uses WPA/WPA2 rather than a captive-portal login page.
4. The board is close enough to the access point.
5. The Wi-Fi firmware message does not report that the board firmware needs updating.

The firmware retries without blocking the face, so you can continue using Serial commands while Wi-Fi is offline.

## Test the interaction states

Type these commands in Serial Monitor:

```text
state idle
state listening
state thinking
state speaking
state happy
state notification
state error
state sleep
state idle
```

The expected visual behavior is:

- `idle`: neutral face, random blinks, occasional small eye glances.
- `listening`: attentive side-eye and listening mouth.
- `thinking`: curious face and slower side-to-side thinking glance.
- `speaking`: animated mouth.
- `happy`: smile, brows, and cheek marks.
- `notification`: attention/surprise face.
- `error`: worried face.
- `sleep`: closed eyes.

## Test the desk dance

Type:

```text
dance rocky
```

The one-axis desk-safe dance will:

1. Center the pan.
2. Show a happy face.
3. Move gently left and right within the configured limits.
4. Alternate curious, happy, and surprised expressions.
5. Return to a happy centered pose.

Stop it at any time with:

```text
stop
```

This is an original small desk-buddy dance. It does not copy a film recording, actor performance, soundtrack, or exact choreography.

When motors are connected, the routine also adds gentle differential-track movements: alternating pivots, a short forward bounce, and a short reverse bounce. Motors are disabled on boot. Test them separately first:

```text
motors on
motors test
motors off
```

Keep the wheels lifted during `motors test`. After that, place the robot on a clear desk area and use:

```text
motors on
drive forward
turn left
turn right
drive backward
motors off
```

Every manual move is timed and automatically stops. `stop` or `motors off` stops the motors immediately.

## Test personality modes

```text
mode calm
mode musical
mode engineer
mode quiet
```

Modes are not Wi-Fi modes and do not turn motors on. They change the instructions sent to the LLM:

| Command | Meaning |
|---|---|
| `mode calm` | Short, warm, relaxing desk-buddy replies; default |
| `mode musical` | Original gentle chirp-like wording and playful curiosity; not a film-voice imitation |
| `mode engineer` | More precise electronics/code answers with voltage, logic-level, grounding, and safety warnings |
| `mode quiet` | Minimal responses and quieter notification behavior |

For example:

```text
mode engineer
ask why must ESP32 GPIO stay at 3.3V
```

## Test notifications

Queue a local notification:

```text
notify firmware build completed
read notifications
```

The robot will show the notification face, print the message, and return to idle.

Notifications are currently a local queue test. Later, the GitHub/Manus relay will send the same `notify ...` command over the network. While a notification is queued, use `read notifications` to display and print it.

## Test the direct LLM call

The UNO itself makes the LLM HTTPS request through the existing `WiFiS3`/`WiFiSSLClient` path. The laptop is not required for this call.

Send:

```text
ask greet me warmly
ask what resistor color code means
ask explain why ESP32 GPIO must not receive 5V
ask dance for me
llm test
```

The UNO shows `thinking` while the request is in progress, validates the returned action, and then selects a safe face, pan, dance, or notification action. It does not allow the model to generate arbitrary GPIO or motor commands.

During a normal response, the OLED shows a speaking visualizer: synchronized equalizer bars inside both eye windows and a scrolling short reply in the bottom strip. The `llm test` command is the simplest first network test. Watch Serial Monitor for `LLM: connecting`, `LLM action`, `LLM reply`, or a precise HTTP/TLS error.

The direct request is intentionally small. The UNO should not send audio, camera frames, long conversation history, or large code files because the RA4M1 has limited memory.

## Test over Wi-Fi from a browser

After the board prints its IP, use:

```text
http://ROBOT_IP/health
http://ROBOT_IP/api/status
http://ROBOT_IP/cmd/state%20listening
http://ROBOT_IP/cmd/state%20thinking
http://ROBOT_IP/cmd/state%20speaking
http://ROBOT_IP/cmd/dance%20rocky
http://ROBOT_IP/cmd/notify%20new%20code%20update
http://ROBOT_IP/input/what%20is%20a%203.3V%20logic%20signal
```

The laptop is only needed for the browser or temporary audio. The LLM request itself is made by the UNO.

## Browser dashboard

The current sketch includes a small phone-friendly dashboard in the UNO's existing HTTP server. It does not require a separate web server, cloud account, or command window.

1. Upload the sketch and wait for `WIFI OK IP: ...` in Serial Monitor.
2. Put the phone or laptop on the same Wi-Fi network.
3. Open `http://ROBOT_IP/` in the browser.
4. Use the buttons for face states, look direction, dance, motor tests, modes, notifications, and LLM questions.

The sketch serves the same dashboard on port 80 and a fallback port 8080. If the first URL times out, try:

```text
http://ROBOT_IP:8080/
```

For the example address `10.142.177.1`, try both `http://10.142.177.1/` and `http://10.142.177.1:8080/`. If both fail, check that the computer is connected to the same `Mrunal` network, not a guest network or a different hotspot. Some hotspots enable client isolation, which allows Internet access but blocks one Wi-Fi client from reaching another. A quick test from the computer is `ping 10.142.177.1`; if ping is blocked, try `curl --noproxy '*' http://10.142.177.1:8080/`.

The dashboard automatically refreshes Wi-Fi, IP, personality mode, motor enablement, and pan angle every five seconds.

The dashboard includes the full control set again: all face expressions, original speaking mouth animation, laugh, mouth overrides, look direction, the longer expression-synchronized dance, motor controls, personality modes, LLM test, notifications, and random check-in controls. Speaking no longer replaces the face with equalizer graphs; it uses the earlier blinking/open-mouth speaking face.

### Clean OLED layout and personality faces

The OLED is now **face-first**. The clock and weather are not permanently printed over the eyes and mouth. During normal interaction the display shows the large face, small Wi-Fi indicator, eye blinks, personality-specific eye sprites, eyebrows, cheeks, and mouth. When the robot has been idle for a longer interval, it briefly shows a randomly selected time, weather, or status card and then returns to the face. Use `time`, `weather`, or `status card` to request a card immediately.

The personalities use different visual identities in addition to different wording and voice delivery: calm uses open friendly eyes, musical uses expressive diagonal eye sprites and cheek accents, engineer uses focused eyes and level brows, quiet uses softer narrowed eyes, and Spartan uses a sharper determined eye style with strong brows and a firm resting mouth.

Each mode now also has a small non-blocking animation profile. Calm makes rare relaxed movements. Musical makes a gentle alternating bounce and rhythm-mark animation. Engineer uses an original Einstein-inspired lab-thinker cue, a focused left-to-right inspection scan, small measurement ticks, and a playful hair-like accent. Quiet uses very subtle breathing-like jitter. Spartan stays centered longer, makes deliberate glances, and uses a restrained crest and firm lower accent. These are original visual character cues; they do not copy a real person's face or voice.

When you switch personalities, the OLED now shows a readable full-screen identity card for about 2.4 seconds before returning to the normal face. Engineer shows an original Einstein-inspired portrait icon with wild hair, round glasses, moustache, and bow tie. Spartan shows a 300-inspired ancient soldier icon with a plume, helmet, eye slit, cheek guards, and shield-like lower silhouette. Musical, Quiet, and Calm also receive cleaner identity cards. The RGB LED changes immediately when a mode is selected: cyan for Engineer, red-orange for Spartan, violet for Musical, white for Quiet, and blue for Calm. During an active state such as thinking, listening, speaking, notification, dancing, or error, the state color takes priority so the LED communicates what the robot is doing. When the robot returns to idle, the personality color returns.

The direct UNO LLM path and the Manus path are different. `LLM_HOST` and `LLM_API_KEY` in `arduino_secrets.h` are for an OpenAI-compatible endpoint because the UNO sends `Authorization: Bearer ...` to `LLM_HOST` and `LLM_PATH`. A Manus API key must not be placed there. Manus uses the `x-manus-api-key` header and the Manus v2 endpoints, so use the laptop relay instead:

```bash
export MANUS_API_KEY='your-rotated-key'
python3 integration_relay/relay_server.py
```

Then send a request to the relay's authenticated `POST /manus/ask` endpoint. A `401 Unauthorized` from `api.openai.com` means the UNO was given a Manus key or another key that is not valid for the OpenAI-compatible endpoint. The firmware now prints this distinction in Serial Monitor.

The pan range is now configured as **0–180 degrees** with 90 degrees as center. This assumes the servo bracket and wiring can physically reach those endpoints. Test `pan 90`, then small movements, with the head disconnected from the mechanism before trying the 0- and 180-degree limits. If the bracket hits an end stop, reduce `PAN_MIN` or `PAN_MAX` immediately.

### RGB status LED

The single built-in LED has been replaced in the firmware logic by an optional external three-channel RGB LED. Use one 220–330 ohm resistor per colour channel and connect the common pin to GND for a common-cathode LED. The default pins are:

| RGB channel | UNO pin |
|---|---:|
| Red | D2 |
| Green | D4 |
| Blue | D8 |

Set `RGB_COMMON_ANODE` to `true` in the sketch if your LED's common pin is connected to 5 V. The status meanings are red blinking for an error, cyan blinking while thinking, purple for listening/waiting for input, amber pulsing for a notification or break, green for speaking/active, magenta for dancing, dim blue for idle, and dim blue sleep indication. The LED channels are simple on/off colour channels on these selected pins; brightness values are used as colour intent, not hardware PWM.

### Browser voice

The browser can also speak replies through the computer's built-in Web Speech API. Calm, musical, engineer, and quiet modes use different rate/pitch targets and voice selection where the browser provides matching voices. This is temporary browser audio; the future robot speaker will need its own TTS/audio path.

The **LLM test** button sends the same deterministic request as the Serial command `llm test`. The **Ask** field sends a question through `/input/...`; the UNO still calls the LLM directly. The dashboard does not contain or expose the API key.

If the dashboard does not load, first open `http://ROBOT_IP/health`. If that also fails, the phone and UNO are not on the same LAN, the printed IP changed, or the browser is using a guest network that cannot reach local devices.

## Xiaozhi-style behavior on the UNO R4

The current UNO implementation adapts the visual interaction pattern from Xiaozhi-style desk buddies:

```text
IDLE → LISTENING → THINKING → SPEAKING → IDLE
```

It already provides idle blinking, listening expression, thinking movement, speaking mouth animation, notifications, and a dashboard. The UNO R4 does not yet have the XIAO project's I2S microphone, MAX98357A amplifier, or speaker, so it cannot be a drop-in replacement for the complete Xiaozhi audio firmware yet. The laptop can temporarily provide microphone and speaker functions.

The future XIAO board can use the same state names and personality behavior when its audio path is added.

## Desk information and routines

The UNO synchronizes time through the Wi-Fi modem's NTP time function and displays a compact clock at the top-left of the OLED. The default timezone is India Standard Time (`TIMEZONE_OFFSET_SECONDS 19800`) and can be changed in `arduino_secrets.h`.

The dashboard's weather button uses the public Open-Meteo geocoding and forecast endpoints from the browser. It sends a compact weather summary back to the UNO, which shows it on the OLED bottom line and in the dashboard status. This weather lookup does not require an API key. The browser must have Internet access.

The built-in status LED uses different patterns: rapid blinking for an error, slower blinking while thinking, pulsing for notifications or a Pomodoro break, and steady on for speaking, happy, or music-vibe activity. Idle is off.

The routine controls are:

```text
pomodoro start
pomodoro stop
water on
water off
water now
day
```

Pomodoro runs a 25-minute work period followed by a five-minute break. Water reminders default to every 45 minutes. The `day` command and dashboard status count questions, check-ins, water reminders, Pomodoro sessions, and the latest activity since boot. This is an in-memory prototype tracker; a future relay/database can persist history across reboots.

`head follow on` makes the pan servo follow left, center, and right eye directions, including idle glances. `head follow off` centers the servo and preserves manual pan control. It is off by default so existing manual servo tests remain predictable.

`vibe on` starts a repeating desk-safe pan/expression rhythm intended to run while music plays. It is not true beat synchronization yet because the current UNO setup has no microphone or audio input. The future XIAO plus microphone path, or a laptop microphone relay, can convert music amplitude/beat events into direct vibe timing.

The **Spartan** personality is the highly driven, wise, motivational mode. Its motto is:

> If it's man-made, I can make it.

It can be selected with `mode spartan` or from the dashboard. The current quote and motivation are original behavior; no film voice or recording is copied.

Outlook mail/calendar notifications and camera-presence check-ins are future relay features. The UNO has neither an Outlook client nor a camera. A private relay can later receive Microsoft Graph events and camera-presence events, then send safe `notify`, `checkin now`, or `/reply/...` commands to the robot.

### Random personal check-ins

Random check-ins are enabled by default, but only run when an LLM API key and usable Wi-Fi are configured. The next check-in occurs after a randomized delay of approximately three to eight minutes, only while the robot is idle. The robot first shows listening for a moment, then thinking, then speaking the result. Use the dashboard or these commands:

```text
checkin now
checkins on
checkins off
```

Each mode has a different approach:

| Mode | Check-in approach | Temporary voice target |
|---|---|---|
| Calm | Gentle break/help question without sounding needy | Slower, warm, lower volume |
| Musical | Lightly playful, original rhythmic wording | Slightly brighter pitch |
| Engineer | Practical progress question plus one actionable electronics/code suggestion | Faster, clear, precise |
| Quiet | Very short, low-pressure question | Slow and quiet |

The dashboard browser voice and `laptop_audio_bridge.py` apply these temporary delivery differences. The UNO itself controls the expression and state; it does not synthesize audio without a speaker/audio subsystem.

### Longer dance

The dance now lasts roughly five to six seconds and matches expressions to movement: happy center pose, curious left/right pivots, surprised forward bounce, laugh/reverse bounce, happy and laughing side sways, then a happy centered finish. Track motors remain optional and disabled by default; without motors, the pan and face choreography still runs.

## ChatGPT Go, Manus, and Arduino API access

A ChatGPT web subscription and a Manus subscription are not automatically API credentials that can be placed in an Arduino sketch. The UNO's HTTPS client needs an API endpoint and an API key issued for that API service, or a private relay that owns the key.

For the current direct test, use an API key configured in the private `arduino_secrets.h`. Never put that key in the dashboard HTML, GitHub, screenshots, or a shared archive.

### Using Manus through a relay

Do not put a Manus API key in the UNO sketch. Use a small HTTPS relay on a laptop, Raspberry Pi, or later persistent host:

```text
UNO / dashboard → relay → https://api.manus.ai/v2/task.create or task.sendMessage
                           ↓
                        Manus reply
                           ↓
                    relay → UNO / dashboard
```

The Manus API uses the `x-manus-api-key` header for an API-key integration. A web subscription login is not itself a key that can be copied into an Arduino file; create or obtain API access through the Manus developer/API workflow. The API base URL is `https://api.manus.ai` and new integrations should use API v2.

For a desk buddy, create one persistent task or use the default agent task shortcut, send short messages containing the user's question and the current personality mode, then poll task messages for the finished response. A production relay should use webhooks rather than frequent polling, verify webhook signatures, keep the key in an environment variable, reject arbitrary GPIO/motor instructions, and return a small safe response such as:

```json
{
  "action": "speaking",
  "reply": "Take a short break and drink some water.",
  "personality": "calm"
}
```

The current UNO direct LLM path remains useful for testing. Manus integration is a separate relay feature; the browser dashboard does not currently call Manus directly and does not expose any API secret.

For a safer final desk buddy, use this architecture:

```text
UNO R4 → private relay → LLM provider
                     ├── ChatGPT-compatible API
                     ├── Manus API/task integration
                     └── Xiaozhi-compatible voice service later
```

The relay keeps long prompts, code files, provider credentials, Manus events, and future Alexa integration off the UNO. For now, the direct UNO call is intentionally short and suitable for testing connectivity and concise electronics questions.

## Temporary laptop microphone and speaker

Use `tools/laptop_audio_bridge.py`.

Text mode:

```bash
python3 -m pip install requests
python3 tools/laptop_audio_bridge.py --robot http://ROBOT_IP
```

Voice mode is optional:

```bash
python3 -m pip install requests SpeechRecognition PyAudio pyttsx3
python3 tools/laptop_audio_bridge.py --robot http://ROBOT_IP --voice
```

The laptop helper does not call the LLM. It sends recognized text to the UNO’s `/input/...` route, reads the result from `/last`, and optionally speaks the short reply with laptop TTS.

## Current limitations

- The UNO R4 remains on the standard RA4M1 + ESP32 bridge architecture.
- The robot has no built-in microphone or speaker yet.
- The current direct LLM response is short and action-oriented.
- Long electronics explanations and generated code should be viewed on a phone/computer, not stored in UNO RAM.
- Alexa and external notifications require a secure cloud relay; the UNO should not be exposed directly to the public internet.

## References

[1]: https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples "Arduino UNO R4 WiFi network examples"
[2]: https://docs.arduino.cc/tutorials/uno-r4-wifi/cheat-sheet "Arduino UNO R4 WiFi ESP32 bridge notes"
[3]: https://www.youtube.com/watch?v=67-NVFrxQDA "Project Hail Mary Rocky reference clip"
