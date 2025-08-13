#include "touch.h"
#include <Arduino.h>

void TouchInput::begin() {
  pinMode(PIN_TOUCH, INPUT_PULLUP);
}

ButtonEvent TouchInput::poll() {
  int v = digitalRead(PIN_TOUCH); // active-low
  uint32_t now = millis();

  if (v != last) {
    if (now - lastChange > DEBOUNCE_MS) {
      lastChange = now;
      last = v;
      if (v == LOW) { pressed = true; t0 = now; }
      else {
        if (pressed) {
          pressed = false;
          uint32_t held = now - t0;
          if (held >= VERY_LONG_PRESS_MS) return BTN_VLONG;
          if (held >= LONG_PRESS_MS)      return BTN_LONG;
          return BTN_SHORT;
        }
      }
    }
  }
  return BTN_NONE;
}
