# Referenced task knowledge base

This file records the external references used for the XIAO integration. It is intentionally concise and contains no credentials.

## ESP32 CI/CD and OTA task

The reference implementation is the public repository [DB_PetBot][1]. Its XIAO path uses a raw ESP32 application binary rather than the UNO R4 `.ota` container. The device fetches a small HTTPS JSON manifest, compares semantic versions, and downloads a newer image with the ESP32 OTA partition mechanism. The first OTA-capable image is uploaded over USB. GitHub Actions compiles the Arduino sketch and GitHub Pages publishes the XIAO binary and manifest. The board keeps USB as the recovery path.

The adapted files are `xiao_bringup/ota_service.cpp`, `xiao_bringup/ota_service.h`, `xiao_bringup/ota_pages_root_ca.h`, `xiao_bringup/device_config.h`, and the two repository workflows under `.github/workflows/`. The service was adjusted to reuse this project's `rocky-xiao` Preferences namespace and `ssid`/`pass` keys.

## Desktop-pet OLED task

The referenced desktop-pet work established a modular split between personality identity, face state, animation frames, and hardware actions. Its recorded assets include Isabella state bitmaps, Spartan character frames, Rocky behavior/frame work, and a reusable modular embedded base. The current XIAO firmware retains the same separation through `Personality`, `FaceMode`, `drawEyes`, `drawPersonalityAccent`, `drawMouth`, head-state poses, and dance/motor functions. Richer bitmap headers can be added later without coupling them to the OTA service.

## Current project boundaries

The XIAO dashboard is local and unauthenticated. It must not be exposed directly to the public internet. OTA is optional and only activates when a local ignored `ota_target.h` points at the user's own GitHub Pages manifest. The microphone stage currently measures PDM signal level; voice recognition, speaker playback, wake-word detection, and an LLM remain separate stages.

## References

[1]: https://github.com/MrunalSwaroop/DB_PetBot "Referenced DB_PetBot XIAO OTA and GitHub Actions implementation"
[2]: https://github.com/TechTalkies/Xiaozhi-for-XiaoESP32S3 "Referenced Xiaozhi-for-XiaoESP32S3 hardware and voice project"
[3]: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/ "Seeed Studio XIAO ESP32-S3 getting started guide"
[4]: https://wiki.seeedstudio.com/xiao_esp32s3_sense_mic/ "Seeed Studio XIAO ESP32-S3 Sense microphone guide"
