// src/modules/fanctrl.cpp
#include "fanctrl.h"
#include <math.h>

// --- internal state ---
static uint32_t s_lastTick    = 0;
static float    s_cmdPct      = 0.0f;  // raw command from curve (%)
static float    s_outPct      = 0.0f;  // filtered & rate-limited (%)
static uint32_t s_kickUntilMs = 0;
static bool     s_wasOn       = false; // for 0% -> >0% edge detection
static bool     s_enabled     = true;  // AUTO mode enabled by default

static inline float clampf(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}
static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Piecewise-linear curve T(°C) -> duty %
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
  for (int i = 0; i < 4; ++i) {
    if (tc <= pt[i + 1].c) {
      const float t = (tc - pt[i].c) / (pt[i + 1].c - pt[i].c);
      return lerp(pt[i].p, pt[i + 1].p, t);
    }
  }
  return pt[4].p;
}

void fanCtrlBegin() {
  s_lastTick    = millis();
  s_cmdPct      = 0.0f;
  s_outPct      = 0.0f;
  s_kickUntilMs = 0;
  s_wasOn       = false;
  s_enabled     = true;    // start in AUTO mode
}

void fanCtrlTick(HostState& host) {
  // If AUTO mode is disabled, do nothing – manual PWM (web UI) is in control
  if (!s_enabled) {
    return;
  }

  const uint32_t now = millis();
  if (now - s_lastTick < FAN_TICK_MS) return;

  const float dt_s = (now - s_lastTick) / 1000.0f;
  s_lastTick = now;

  // 1) Command from Dallas temperature
  const float tC        = host.local_temp_c;
  const bool  tempValid = !isnan(tC);

  if (tempValid) {
    host.fan_last_valid_ms = now;
    s_cmdPct = curveDutyFromTemp(tC);
  } else {
    // Sensor invalid: after timeout, go to safe duty
    if (!host.fan_last_valid_ms || (now - host.fan_last_valid_ms > FAN_INVALID_HOLD_MS)) {
      s_cmdPct = FAN_SAFE_PCT;
    }
  }

  // 2) Rate limit (±FAN_RATE_PCT_PER_S %/s)
  const float rate    = FAN_RATE_PCT_PER_S * dt_s;
  const float limited = clampf(s_cmdPct, s_outPct - rate, s_outPct + rate);

  // 3) Smoothing (EMA with Q8 alpha)
  const float alpha = (float)FAN_ALPHA_Q8 / 256.0f;  // e.g., 0.25
  s_outPct = (1.0f - alpha) * s_outPct + alpha * limited;

  // 4) Deadzone: anything < 8% becomes 0% (allow exactly 8% to pass)
  const float DEADZONE_PCT = 8.0f;
  if (s_outPct < DEADZONE_PCT) {
    s_outPct = 0.0f;
  }

  // 5) Start-kick: detect 0% -> >0% edge and kick briefly
  float applyPct = s_outPct;

  const bool isOnNow = (s_outPct > 0.1f);
  const bool wasOn   = s_wasOn;
  if (!wasOn && isOnNow) {
    // rising edge: OFF -> ON
    s_kickUntilMs = now + FAN_KICK_MS;
  }
  s_wasOn = isOnNow;

  if ((int32_t)(s_kickUntilMs - now) > 0 && isOnNow) {
    applyPct = FAN_KICK_PCT;
  }

  // 6) Clamp and send to FAN1 (0..100%)
  if (applyPct < 0.0f)   applyPct = 0.0f;
  if (applyPct > 100.0f) applyPct = 100.0f;
  fan1PwmSetPercent((uint8_t)lroundf(applyPct));

  // 7) Telemetry for pages
  host.fan_duty_cmd  = s_cmdPct;
  host.fan_duty_filt = s_outPct;
  host.fan_active    = (applyPct > 0.1f) ? 1 : 0;
}

// --- enable/disable API used by web_server.cpp ---

bool fanCtrlGetEnabled() {
  return s_enabled;
}

void fanCtrlSetEnabled(bool enabled) {
  s_enabled = enabled;
  if (enabled) {
    // When re-enabling AUTO, reset tick base so we don't jump a huge dt
    s_lastTick = millis();
  }
}
