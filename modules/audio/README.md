# Audio Module

The current MAX98357A implementation provides the verified I2S initialization and low-volume tone test in `firmware/xiao_esp32s3_sense/xiao_esp32s3_sense.ino`. The next extraction should move PCM playback, tone generation, volume limits, and audio-state events here.

The module must preserve GPIO2/D1 data, GPIO3/D2 BCLK, GPIO4/D3 LRCK, the onboard PDM microphone reservation, and the bridge-tied speaker-output warning.
