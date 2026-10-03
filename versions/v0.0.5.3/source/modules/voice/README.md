# Voice Module

This boundary is reserved for push-to-talk, wake-word detection, speech recognition, response generation, and speech playback adapters. The current firmware intentionally stops at microphone level monitoring and speaker tone testing.

A voice backend must emit the existing listening, thinking, speaking, and error states. Credentials and cloud endpoints must not be placed in character or hardware modules.


## Wake-name boundary

`wake_name.h` defines the policy and configuration boundary. v0.0.5.2 exposes `wake simulate` for testing the engagement path, but does not pretend that a PDM amplitude meter is speech recognition. A future local wake-word engine, relay, or Xiaozhi adapter should invoke the engagement function after recognizing the configured name.
