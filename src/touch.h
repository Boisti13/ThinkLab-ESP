// touch.h  (top of file)
#pragma once
#include <Arduino.h>   // <-- add this
#include "config.h"

enum ButtonEvent { BTN_NONE, BTN_SHORT, BTN_LONG, BTN_VLONG };

class TouchInput {
public:
  void begin();
  ButtonEvent poll();
private:
  int       last = HIGH;
  uint32_t  lastChange = 0;
  bool      pressed = false;
  uint32_t  t0 = 0;
};
