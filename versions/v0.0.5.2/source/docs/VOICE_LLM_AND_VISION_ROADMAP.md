# Deskbot Voice, LLM, Xiaozhi, and Camera Roadmap

## Current release: v0.0.5.2

The first goal is electrical and local validation:

1. Read the Sense onboard PDM microphone.
2. Show a stable activity meter on the OLED and dashboard.
3. Route captured PCM to the MAX98357A speaker through a bounded loopback test.
4. Control software volume without changing the hardware pin map.
5. Test the wake-engagement path manually with `wake simulate`.

## Why spoken-name recognition is not enabled yet

Microphone amplitude only answers “is there sound?” It cannot recognize a word. Reliable hands-free engagement needs a wake-word model or a speech-recognition backend. That component must be added behind the voice module boundary so the hardware loopback remains independently testable.

## Planned stages

### Stage A — local wake word

Add a small local wake-word engine or a supported offline recognizer. It should emit only a boolean/name event such as `wake recognized rocky`; the main firmware then calls the existing engagement path. No API key belongs in the XIAO firmware.

### Stage B — voice activity and response audio

Add voice-activity detection, capture windows, decoded response playback, interrupt/stop behavior, and expressive face states. Loopback must remain available as a diagnostic mode.

### Stage C — LLM integration

Use a separate relay or a carefully bounded HTTPS client. The preferred scalable boundary is:

- XIAO: wake event, audio capture/playback, face/motion state, local dashboard.
- Relay: provider authentication, LLM request, speech-to-text, text-to-speech, rate limits, and logs.
- GitHub: firmware releases and OTA only; never store provider credentials in the repository or firmware image.

### Stage D — Xiaozhi account integration

The linked Xiaozhi account is a future integration target, not part of v0.0.5.2. Before connecting it, verify the account’s current device protocol, authentication method, audio format, and whether the service expects a relay. Implement it as a replaceable voice backend rather than coupling the OLED, motor, or personality code to the service.

### Stage E — camera follow-me behavior

Camera follow should be a separate vision module with explicit safety limits:

- detect a face/person locally;
- estimate horizontal position only;
- apply a dead band so the servo does not jitter;
- clamp pan to the configured 0–180° range;
- stop following when confidence is low;
- never drive motors from camera tracking until a separate low-speed safety test is passed.

The current camera snapshot/live feature remains available and is not silently converted into autonomous motion.

## Personality and behavior integration

Personalities should produce intent/state events such as `listening`, `thinking`, `speaking`, `happy`, `curious`, or `error`. Face modules render those states; motion modules decide whether a safe head gesture is appropriate; voice modules decide how audio is produced. This preserves the modularity needed to add or remove Rocky, Isabella, Spartan, or future personalities independently.
