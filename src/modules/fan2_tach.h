#pragma once
#include <Arduino.h>
#include "config.h"

#if USE_FAN2_TACH
  void fan2TachBegin();
  void fan2TachTick();     // call in loop()
  int  fan2TachGetRPM();   // -1 unknown, 0 stopped, >0 rpm
#else
  inline void fan2TachBegin() {}
  inline void fan2TachTick() {}
  inline int  fan2TachGetRPM() { return -1; }
#endif
