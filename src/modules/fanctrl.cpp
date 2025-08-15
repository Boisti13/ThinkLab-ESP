#include "fanctrl.h"

static uint32_t s_lastTick = 0;
static float    s_cmdPct   = 0.0f;   // raw command (pre-filters)
static float    s_outPct   = 0.0f;   // filtered & rate-limited
static uint32_t s_kickUntilMs = 0;

static inline float clampf(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}
static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Piecewise-linear curve T(°C) -> duty%
static float curveDutyFromTemp(float tc) {
  struct P { float c, p; };
  const P pt[] = {
    {FC_PT1_C, FC_PT1_P},
    {FC_PT2_C, FC_PT2_P},
    {FC_PT3_C, FC_PT3_P},
    {FC_PT4_C, FC_PT4_P},
    {FC_PT5_C, FC_PT5_P}
  };
  if (tc <= pt[0].c) return pt[0].p;
  for (int i=0; i<4; ++i) {
    if (tc <= pt[i+1].c) {
      float t = (tc - pt[i].c) / (pt[i+1].c - pt[i].c);
      return lerp(pt[i].p, pt[i+1].p, t);
    }
  }
  return pt[4].p;
}

void fanCtrlBegin() {
  s_lastTick = millis();
  s_cmdPct   = 0.0f;
  s_outPct   = 0.0f;
  s_kickUntilMs = 0;
}

void fanCtrlTick(HostState& host) {
  uint32_t now = millis();
  if (now - s_lastTick < FAN_TICK_MS) return;
  float dt_s = (now - s_lastTick) / 1000.0f;
  s_lastTick = now;

  // 1) Command from Dallas temperature
  const float tC = host.local_temp_c;
  const bool tempValid = !isnan(tC);
  if (tempValid) {
    host.fan_last_valid_ms = now;
    float cmd = curveDutyFromTemp(tC);
    if (cmd > 0 && cmd < FAN_MIN_DUTY_PCT) cmd = FAN_MIN_DUTY_PCT;  // floor
    if (cmd < FAN_OFF_BELOW_PCT) cmd = 0;                            // off-below
    if (s_outPct <= 0.1f && cmd > 0.1f) s_kickUntilMs = now + FAN_KICK_MS; // start-kick
    s_cmdPct = cmd;
  } else {
    // Hold previous; after long invalid, go safe
    if (!host.fan_last_valid_ms || (now - host.fan_last_valid_ms > FAN_INVALID_HOLD_MS)) {
      s_cmdPct = FAN_SAFE_PCT;
    }
  }

  // 2) Rate limit (±FAN_RATE_PCT_PER_S %/s)
  const float rate = FAN_RATE_PCT_PER_S * dt_s;
  const float limited = clampf(s_cmdPct, s_outPct - rate, s_outPct + rate);

  // 3) Smoothing (EMA)
  const float alpha = (float)FAN_ALPHA_Q8 / 256.0f;  // e.g., 0.25
  s_outPct = (1.0f - alpha) * s_outPct + alpha * limited;

  // 4) Enforce thresholds after filtering
  if (s_outPct > 0 && s_outPct < FAN_MIN_DUTY_PCT) s_outPct = FAN_MIN_DUTY_PCT;
  if (s_outPct < FAN_OFF_BELOW_PCT) s_outPct = 0;

  // 5) Start-kick override
  float applyPct = s_outPct;
  if ((int32_t)(s_kickUntilMs - now) > 0) applyPct = FAN_KICK_PCT;

  // 6) Output PWM
  fan1PwmSetPercent(applyPct);

  // 7) Telemetry for pages
  host.fan_duty_cmd  = s_cmdPct;
  host.fan_duty_filt = s_outPct;
  host.fan_active    = (applyPct > 0.1f) ? 1 : 0;
}
