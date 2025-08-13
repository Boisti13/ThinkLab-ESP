#pragma once
#include <Arduino.h>

class DallasProbe {
public:
  void begin();
  void tick();                 // non-blocking
  float lastC() const { return lastC_; }
private:
  float lastC_ = NAN;
  enum { IDLE, CONVERTING } state = IDLE;
  uint32_t tStart = 0;
  uint32_t lastSampleMs = 0;
};
