# Core Module

This boundary is reserved for shared face states, personality registration, event names, and feature capability flags. Keep these contracts independent from OLED geometry, motor pins, Wi-Fi credentials, and audio drivers so modules can be added or removed without changing unrelated hardware.

`release_notes.h` is the data-only release registry. Update its version, summary, and change list for each firmware release so the local dashboard can explain what is installed through `/api/changes`.
