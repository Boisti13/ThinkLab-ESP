// src/modules/fanctrl.cpp
#include "fanctrl.h"
#include <math.h>

// ---- Low-end behavior (no flapping) ----
// OFF threshold: anything below this is OFF
#ifndef FAN_OFF_BELOW_PCT
  #define FAN_OFF_BELOW_PCT 8     // allow 8% to pass (strictly <8 turns OFF)
#endif
// ON threshold: must reach this to turn ON from OFF (hysteresis)
#ifndef FAN_ON_ABOVE_PCT
  #define FAN_ON_ABOVE_PCT 12     // small gap to avoid chattering
#endif
// Min dwell times so we don't flip states too fast (ms)
#ifndef FAN_MIN_ON_MS
  #define FAN_MIN_ON_MS  4000
#endif
#ifndef FAN_MIN_OFF_MS
  #define FAN_MIN_OFF_MS 4000
#endif

static uint32_t s_lastTick     = 0;
static float    s_cmdPct       = 0.0f;  // raw command from curve (%)
static float    s_outPct       = 0.0f;  // filtered & rate-limited output (%)
static uint32_t s_kickUntilMs  = 0;
static bool     s_onLatched    = false; // ON/OFF latch
static uint32_t s_stateChanged = 0;     // last time we toggled ON/OFF

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
  for (int i = 0; i < 4; ++i) {
    if (tc <= pt[i + 1].c) {
      const float t = (tc - pt[i].c) / (pt[i + 1].c - pt[i].c);
      return lerp(pt[i].p, pt[i + 1].p, t);
    }
  }
  return pt[4].p;
}

void fanCtrlBegin() {
  s_lastTick     = millis();
  s_cmdPct       = 0.0f;
  s_outPct       = 0.0f;
  s_kickUntilMs  = 0;
  s_onLatched    = false;
  s_stateChanged = s_lastTick;
}

void fanCtrlTick(HostState& host) {
  const uint32_t now = millis();
  if (now - s_lastTick < FAN_TICK_MS) return;

  const float dt_s = (now - s_lastTick) / 1000.0f;
  s_lastTick = now;

  // 1) Command from Dallas temperature (no floors here)
  const float tC = host.local_temp_c;
  const bool  tempValid = !isnan(tC);

  if (tempValid) {
    host.fan_last_valid_ms = now;
    s_cmdPct = curveDutyFromTemp(tC);
  } else {
    // Sensor invalid: after timeout, go safe and latch ON once
    if (!host.fan_last_valid_ms || (now - host.fan_last_valid_ms > FAN_INVALID_HOLD_MS)) {
      s_cmdPct = FAN_SAFE_PCT;
      if (!s_onLatched) {
        s_onLatched = true;
        s_stateChanged = now;
        s_kickUntilMs = now + FAN_KICK_MS;
      }
    }
  }

  // 2) Rate limit (±FAN_RATE_PCT_PER_S %/s)
  const float rate    = FAN_RATE_PCT_PER_S * dt_s;
  const float limited = clampf(s_cmdPct, s_outPct - rate, s_outPct + rate);

  // 3) Smoothing (EMA with Q8 alpha)
  const float alpha = (float)FAN_ALPHA_Q8 / 256.0f;
  s_outPct = (1.0f - alpha) * s_outPct + alpha * limited;

  // 4) State machine with hysteresis + dwell
  const float offThr = (float)FAN_OFF_BELOW_PCT; // strictly < offThr -> OFF
  const float onThr  = (float)FAN_ON_ABOVE_PCT;  // >= onThr -> can turn ON
  float applyPct = s_outPct;

  if (!s_onLatched) {
    // OFF state
    applyPct = 0.0f;
    // Require dwell time before considering turn-on
    const bool dwellOk = (now - s_stateChanged) >= FAN_MIN_OFF_MS;
    if (dwellOk && s_cmdPct >= onThr) {
      s_onLatched = true;
      s_stateChanged = now;
      s_kickUntilMs = now + FAN_KICK_MS; // one-shot kick
      applyPct = onThr; // seed at least ON threshold after the kick ends
    }
  } else {
    // ON state
    // Never go below OFF threshold while latched ON
    if (applyPct < offThr) applyPct = offThr;

    // Turn OFF only if under OFF threshold and dwell elapsed
    const bool dwellOk = (now - s_stateChanged) >= FAN_MIN_ON_MS;
    if (dwellOk && s_cmdPct < offThr) {
      s_onLatched = false;
      s_stateChanged = now;
      applyPct = 0.0f;
      s_kickUntilMs = 0;
    }
  }

  // 5) Start-kick (brief) — only while kick window active
  if ((int32_t)(s_kickUntilMs - now) > 0) {
    applyPct = FAN_KICK_PCT;
  }

  // 6) Clamp and send to FAN1 (0..100)
  if (applyPct < 0.0f)   applyPct = 0.0f;
  if (applyPct > 100.0f) applyPct = 100.0f;
  fan1PwmSetPercent((uint8_t)lroundf(applyPct));

  // 7) Telemetry
  host.fan_duty_cmd  = s_cmdPct;
  host.fan_duty_filt = s_outPct;
  host.fan_active    = (applyPct > 0.1f) ? 1 : 0;
}
