#pragma once

class VoiceBackend {
 public:
  virtual ~VoiceBackend() = default;
  virtual bool begin() = 0;
  virtual bool listening() const = 0;
  virtual void update() = 0;
};
