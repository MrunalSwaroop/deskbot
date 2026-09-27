# XIAO Deskbot Modules

This directory contains replaceable feature modules used by `../firmware/xiao_esp32s3_sense/xiao_esp32s3_sense.ino`.

## Current modules

- `faces/isabella_character.h` — Isabella renderer and state behavior.
- `faces/isabella_state_bitmaps.h` — Isabella 128×64 bitmap frames.
- `faces/spartan_character.h` — Spartan procedural renderer and poses.
- `faces/rocky_bump_frames.h` — Rocky dance frame table.
- `faces/rocky_body_language.h` — supplied Rocky body poses, state parser, and animation adapter.

Isabella’s base bitmap reel remains in `isabella_state_bitmaps.h`; `isabella_character.h` adds distinct animated overlays for curious, thinking, sad, laughing, surprised, and speaking states.

## Module contract

A module may expose:

- a state enum;
- a setter or name parser;
- a renderer that draws only to the supplied global OLED display;
- a deterministic test command;
- optional animation frame state.

A module must not own Wi-Fi credentials, OTA URLs, motor power, servo power, camera initialization, or the microphone and amplifier pins. Those responsibilities remain in the integration and hardware layers.

When adding a module, update `docs/MODULAR_ARCHITECTURE.md`, add a test command, compile the complete sketch, and record the change in `CHANGE_SUMMARY.md`.
