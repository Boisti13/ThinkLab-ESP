#include "touch.h"

static inline bool isActiveLevel(int lvl) {
#if TOUCH_ACTIVE_LOW
  return lvl == LOW;
#else
  return lvl == HIGH;
#endif
}

void TouchInput::begin() {
#if TOUCH_ACTIVE_LOW
  pinMode(TOUCH_PIN, INPUT_PULLUP);
#else
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);
#endif
  lastLevel = digitalRead(TOUCH_PIN);
  lastChange = millis();
  pressT0 = 0;
  pressed = false;
  lastTapMs = 0;
  waitingSecondTap = false;
}

ButtonEvent TouchInput::poll() {
  const uint32_t now = millis();
  int lvl = digitalRead(TOUCH_PIN);

  if (lvl != lastLevel) {
    if (now - lastChange >= TOUCH_DEBOUNCE_MS) {
      lastLevel = lvl;
      lastChange = now;

      if (isActiveLevel(lvl)) {
        // pressed
        pressed = true;
        pressT0 = now;
      } else {
        // released
        if (pressed) {
          pressed = false;
          const uint32_t held = now - pressT0;

          // Long-press?
          if (held >= TOUCH_LONG_MS) {
            waitingSecondTap = false;
            lastTapMs = 0;
            return ButtonEvent::LongPress;
          }

          // Double-tap?
          if (waitingSecondTap && (now - lastTapMs) <= TOUCH_DBL_MS) {
            waitingSecondTap = false;
            lastTapMs = 0;
            return ButtonEvent::DoubleTap;
          }

          // Single tap, arm double-tap wait
          waitingSecondTap = true;
          lastTapMs = now;
          return ButtonEvent::Tap;
        }
      }
    }
  } else {
    // timeout the double-tap wait (no event)
    if (waitingSecondTap && (now - lastTapMs) > TOUCH_DBL_MS) {
      waitingSecondTap = false;
    }
  }

  return ButtonEvent::None;
}
