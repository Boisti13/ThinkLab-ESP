#include "fan1_pwm.h"

#if USE_FAN1
static const int kLedcChannel = 0;
static const int kLedcResBits = 10;                   // 0..1023 steps
static const int kLedcMax     = (1 << kLedcResBits) - 1;
static uint8_t   s_pct        = 0;

void fan1PwmBegin() {
  pinMode(FAN1_PWM_PIN, OUTPUT);
  ledcSetup(kLedcChannel, FAN1_PWM_FREQ_HZ, kLedcResBits);
  ledcAttachPin(FAN1_PWM_PIN, kLedcChannel);
  fan1PwmSetPercent(FAN1_DEFAULT_PWM_PCT);
}

void fan1PwmSetPercent(uint8_t pct) {
  if (pct > 100) pct = 100;
  if (pct > 0 && pct < FAN1_MIN_DUTY_PCT) pct = FAN1_MIN_DUTY_PCT;  // avoid dead zone
  s_pct = pct;
  uint32_t duty = (uint32_t)((uint32_t)pct * kLedcMax + 50) / 100; // round
  // Intel 4-wire PWM: “high” on MCU pulls line LOW via transistor → mapping is correct
  ledcWrite(kLedcChannel, duty);
}

uint8_t fan1PwmGetPercent() { return s_pct; }
#endif
