#pragma once
#include <Arduino.h>
#include "config.h"

enum class ButtonEvent : uint8_t { None, Tap, DoubleTap, LongPress };

class TouchInput {
public:
  void begin();
  ButtonEvent poll();  // non-blocking; call every loop()

private:
  int      lastLevel = HIGH;
  uint32_t lastChange = 0;
  uint32_t pressT0 = 0;
  bool     pressed = false;

  // double-tap tracking
  uint32_t lastTapMs = 0;
  bool     waitingSecondTap = false;
};
