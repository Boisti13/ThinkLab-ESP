#pragma once
#include <Arduino.h>
#include "config.h"

#if USE_FAN1
  void fan1TachBegin();
  void fan1TachTick();     // call in loop()
  int  fan1TachGetRPM();   // -1 = unknown, 0 = stopped, >0 = rpm
#else
  inline void fan1TachBegin() {}
  inline void fan1TachTick() {}
  inline int  fan1TachGetRPM() { return -1; }
#endif
