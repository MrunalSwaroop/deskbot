# Rocky-Wall-E integration relay scaffold

This folder contains a **prototype scaffold**, not a public production deployment. It does not reflash the UNO R4 or change the current WiFiS3 architecture.

The relay demonstrates the event-to-notification and Alexa intent paths:

- GitHub webhook: code updates and build status.
- Manus webhook adapter: task/message update placeholder.
- Alexa Custom Skill adapter: dance, look, and notification intents.
- Local notification endpoint for testing.

It also includes `POST /manus/ask`. That endpoint uses `MANUS_API_KEY` from the relay process environment, creates a private Manus v2 task on first use, reuses the task ID for later prompts, polls `task.listMessages`, and forwards the short reply to the UNO's `/reply/...` endpoint. No Manus key is stored in the Arduino sketch or dashboard.

**Credential safety:** do not use an API key that has been pasted into chat, screenshots, GitHub, or a shared file. Revoke/rotate an exposed key in the provider console, then export the replacement only in the relay process environment. The relay intentionally does not accept a key from the browser or the UNO.

For a first Manus test, run the relay on the laptop that can reach the UNO over Wi-Fi:

```bash
export RELAY_SECRET='a-long-local-secret'
export ROBOT_URL='http://192.168.31.72:8080'
export MANUS_API_KEY='your-rotated-manus-key'
python3 relay_server.py
```

Then, from another terminal on the laptop:

```bash
curl -X POST http://127.0.0.1:5060/manus/ask \
  -H 'Content-Type: application/json' \
  -H 'X-Relay-Secret: a-long-local-secret' \
  -d '{"personality":"engineer","prompt":"Give me one short electronics safety tip."}'
```

The UNO's `llm test` command is not a Manus test. It tests only the direct OpenAI-compatible endpoint configured in `arduino_secrets.h`.

The `POST /presence` endpoint is a future camera adapter. An authenticated camera service can send `{"present":true}`; the relay will trigger `checkin now` at most once every ten minutes. The current UNO and XIAO-less hardware setup has no camera, so this endpoint is only a safe integration seam.

## Current prototype limitation

The sample relay forwards to the UNO using a local `ROBOT_URL`. That works only when the relay can reach the robot on the same LAN. A cloud deployment must not attempt to open the robot’s HTTP server to the public internet.

The production design should use an **outbound robot connection**:

```text
UNO R4 → secure MQTT/WebSocket/HTTPS polling connection → relay
GitHub / Manus / Alexa → relay → existing outbound robot connection
```

The relay must add:

1. HTTPS hosting.
2. Device authentication and rotation.
3. GitHub HMAC signature verification with the real webhook secret.
4. Manus webhook public-key signature verification according to the configured API version.
5. Durable notification storage and deduplication.
6. Alexa request verification using the official Alexa request-signature rules.
7. Per-device authorization and a quiet-hours policy.

Do not expose the UNO’s unauthenticated port-80 server directly to the internet.

## Prototype run

```bash
python3 -m pip install flask requests
export RELAY_SECRET='replace-this'
export ROBOT_URL='http://10.142.177.1:8080'
export MANUS_API_KEY='your-manus-api-key'
python3 relay_server.py
```

Test the Manus path:

```bash
curl -X POST http://127.0.0.1:5060/manus/ask \\
  -H 'Content-Type: application/json' \\
  -H 'X-Relay-Secret: replace-this' \\
  -d '{"prompt":"Give Rocky a short calm electronics tip."}'
```

Test notification:

```bash
curl -X POST http://127.0.0.1:5060/notify \\
  -H 'Content-Type: application/json' \\
  -H 'X-Relay-Secret: replace-this' \\
  -d '{"message":"firmware build completed"}'
```

The current environment did not contain a `MANUS_API_KEY`, so the adapter is present but not activated. Supply the key on the machine running the relay; never put it in `arduino_secrets.h`. The relay remains a prototype and should be placed behind HTTPS with authentication before remote use.
