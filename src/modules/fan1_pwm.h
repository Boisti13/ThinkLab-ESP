#pragma once
#include <Arduino.h>
#include "config.h"

#if USE_FAN1
  void fan1PwmBegin();
  void fan1PwmSetPercent(uint8_t pct);  // 0..100 (clamped; applies FAN1_MIN_DUTY_PCT if >0)
  uint8_t fan1PwmGetPercent();
#else
  inline void   fan1PwmBegin() {}
  inline void   fan1PwmSetPercent(uint8_t) {}
  inline uint8_t fan1PwmGetPercent() { return 0; }
#endif
