# Network Module

The current Wi-Fi provisioning and local dashboard remain in `firmware/xiao_esp32s3_sense/xiao_esp32s3_sense.ino` because they are part of the verified bring-up path. This directory is the extraction boundary for local HTTP/WebSocket control, dashboard authentication, and room-to-room configuration.

Wi-Fi credentials belong in ESP32 NVS through the setup page, not in Git or firmware source.
