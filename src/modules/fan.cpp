#include "config.h"
#if USE_FANCTRL
#include "fan.h"
#include <Arduino.h>

void FanController::begin() {
  // LEDC PWM setup for GPIO PIN_FAN_PWM; tacho on PIN_FAN_TACH later
  ledcSetup(0, 25000, 8); // channel 0, 25kHz, 8-bit
  ledcAttachPin(PIN_FAN_PWM, 0);
  setDuty(0);
}
void FanController::tick() {
  // future: read tacho on interrupt/poll, adjust duty
}
void FanController::setDuty(uint8_t pct) {
  duty_ = pct;
  uint32_t val = map(pct, 0, 100, 0, 255);
  ledcWrite(0, val);
}
#endif
