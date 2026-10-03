# Motion Module

The current verified servo and DRV8833 implementation remains in `firmware/xiao_esp32s3_sense/xiao_esp32s3_sense.ino` while the electrical pin map is still being brought up. The next extraction should move bounded servo poses, differential-drive commands, motor inversion, timeout stopping, and dance choreography here without changing character renderers.

Every motion function must have a timeout, a stop path, a lifted-robot test, and a documented power assumption.
