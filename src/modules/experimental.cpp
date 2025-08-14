#include "experimental.h"

#if USE_EXPERIMENTAL
  #include "modules/fan1_pwm.h"   // fan1 PWM

  static uint32_t s_t0 = 0;
  static int8_t   s_lastPct = -1;

  static inline uint8_t clampPct(int v) {
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    return (uint8_t)v;
  }

  void experimentalBegin() {
  #if EXP_TEST_FAN1_SWEEP
    s_t0 = millis();
    s_lastPct = -1;
  #else
    #if EXP_TEST_FAN1_PWM
      fan1PwmSetPercent(FAN1_TEST_PWM_PCT);
      s_lastPct = (int8_t)FAN1_TEST_PWM_PCT;
    #endif
    s_t0 = millis();
  #endif
  }

  void experimentalTick(UiState& /*ui*/) {
  #if EXP_TEST_FAN1_SWEEP
    const uint32_t now = millis();
    const uint32_t T   = (FAN1_SWEEP_PERIOD_MS < 1000) ? 1000 : (uint32_t)FAN1_SWEEP_PERIOD_MS;
    const int      lo  = FAN1_SWEEP_MIN_PCT;
    const int      hi  = FAN1_SWEEP_MAX_PCT;
    const int      span = (hi > lo) ? (hi - lo) : 0;

    if (span > 0) {
      uint32_t phase = (now - s_t0) % T;
      uint32_t half  = T / 2;
      float frac = (phase < half)
                   ? (float)phase / (float)half
                   : 1.0f - (float)(phase - half) / (float)half;

      int pct = lo + (int)(span * frac + 0.5f);
      pct = clampPct(pct);

      if (pct != s_lastPct) {
        fan1PwmSetPercent((uint8_t)pct);
        s_lastPct = (int8_t)pct;
      }
    } else {
      if (lo != s_lastPct) {
        fan1PwmSetPercent((uint8_t)clampPct(lo));
        s_lastPct = (int8_t)clampPct(lo);
      }
    }
  #else
    // fixed test applied in begin
  #endif
  }

#endif // USE_EXPERIMENTAL
