# Deskbot Hardware

The active hardware target is the Seeed XIAO ESP32-S3 Sense. The complete diagrams and pin tables are in `wiring/`.

The physical build keeps the OLED, servo, DRV8833/N90 motors, MAX98357A amplifier, onboard PDM microphone, and Sense camera on the verified pin map. Motor and servo power remain external and all grounds are common. The local dashboard is not exposed directly to the public internet.
