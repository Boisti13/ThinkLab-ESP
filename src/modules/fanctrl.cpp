// src/modules/fanctrl.cpp
#include "fanctrl.h"
#include <math.h>

// --- internal state ---
static uint32_t s_lastTick    = 0;
static float    s_cmdPct      = 0.0f;  // raw command from curve (%)
static float    s_filtPct     = 0.0f;  // rate-limited + smoothed command (%), never zeroed
static float    s_outPct      = 0.0f;  // what goes to the fan (%)
static uint32_t s_kickUntilMs = 0;
static bool     s_on          = false; // fan running (on/off with hysteresis)
static uint32_t s_stateSince  = 0;     // when s_on last changed
static bool     s_enabled     = true;  // AUTO mode enabled by default
static uint32_t s_manualSince = 0;     // when manual mode started

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
  s_filtPct     = 0.0f;
  s_outPct      = 0.0f;
  s_kickUntilMs = 0;
  s_on          = false;
  s_stateSince  = s_lastTick - FAN_MIN_OFF_MS;   // may switch on at once
  s_enabled     = true;                         // start in AUTO mode
}

static void switchOn(uint32_t now) {
  s_on = true;
  s_stateSince = now;
  s_kickUntilMs = now + FAN_KICK_MS;   // brief full power so it surely starts
}

void fanCtrlTick(HostState& host) {
  const uint32_t now = millis();

  // Manual mode (web page): keep the set duty, but give control back to the
  // curve after FAN_MANUAL_MAX_MS or when the case gets hot.
  if (!s_enabled) {
    const float tC = host.local_temp_c;
    const bool tooHot  = !isnan(tC) && tC >= (float)FAN_MANUAL_OVERRIDE_C;
    const bool expired = (now - s_manualSince) >= FAN_MANUAL_MAX_MS;
    if (!tooHot && !expired) return;
    fanCtrlSetEnabled(true);
  }

  if (now - s_lastTick < FAN_TICK_MS) return;

  float dt_s = (now - s_lastTick) / 1000.0f;
  if (dt_s > 2.0f) dt_s = 2.0f;   // a long loop stall must not make the filter jump
  s_lastTick = now;

  // 1) Command from the case temperature (NaN = no valid reading)
  const float tC        = host.local_temp_c;
  const bool  tempValid = !isnan(tC);
  bool safe = false;

  if (tempValid) {
    host.fan_last_valid_ms = now;
    s_cmdPct = curveDutyFromTemp(tC);
  } else if (!host.fan_last_valid_ms || (now - host.fan_last_valid_ms > FAN_INVALID_HOLD_MS)) {
    // No sensor yet, or lost for FAN_INVALID_HOLD_MS: run at a safe duty
    s_cmdPct = FAN_SAFE_PCT;
    safe = true;
  }

  // 2) Rate limit (±FAN_RATE_PCT_PER_S %/s), then 3) smoothing (EMA, Q8 alpha).
  //    The filter runs on the command alone; switching off below never resets it.
  const float rate    = FAN_RATE_PCT_PER_S * dt_s;
  const float limited = clampf(s_cmdPct, s_filtPct - rate, s_filtPct + rate);
  const float alpha   = (float)FAN_ALPHA_Q8 / 256.0f;
  s_filtPct = (1.0f - alpha) * s_filtPct + alpha * limited;

  // 4) On/off with hysteresis and minimum on/off times, so the fan doesn't
  //    chatter (and kick) around the curve's low end. Safe mode starts it at once.
  if (!s_on) {
    if (safe || (s_filtPct >= FAN_ON_ABOVE_PCT && now - s_stateSince >= FAN_MIN_OFF_MS)) switchOn(now);
  } else if (!safe && s_filtPct < FAN_OFF_BELOW_PCT && now - s_stateSince >= FAN_MIN_ON_MS) {
    s_on = false;
    s_stateSince = now;
  }

  // 5) Output: off, or at least FAN1_MIN_DUTY_PCT; full power during the start kick
  float applyPct = 0.0f;
  if (s_on) {
    applyPct = s_filtPct < FAN1_MIN_DUTY_PCT ? (float)FAN1_MIN_DUTY_PCT : s_filtPct;
    if ((int32_t)(s_kickUntilMs - now) > 0) applyPct = FAN_KICK_PCT;
  }
  if (applyPct > 100.0f) applyPct = 100.0f;
  s_outPct = applyPct;
  fan1PwmSetPercent((uint8_t)lroundf(applyPct));

  // 6) Telemetry for pages and status.json
  host.fan_duty_cmd  = s_cmdPct;
  host.fan_duty_filt = s_on ? s_filtPct : 0.0f;
  host.fan_active    = s_on ? 1 : 0;
}

// --- enable/disable API used by web_server.cpp ---

bool fanCtrlGetEnabled() {
  return s_enabled;
}

void fanCtrlSetManual(uint8_t pct) {
  if (pct > 100) pct = 100;
  s_enabled = false;
  s_manualSince = millis();
  fan1PwmSetPercent(pct);
  // continue smoothly from here when AUTO resumes
  s_filtPct = pct;
  s_outPct = pct;
  if ((pct > 0) != s_on) { s_on = pct > 0; s_stateSince = s_manualSince; }
}

uint32_t fanCtrlManualLeftS() {
  if (s_enabled) return 0;
  const uint32_t used = millis() - s_manualSince;
  return used >= FAN_MANUAL_MAX_MS ? 0 : (FAN_MANUAL_MAX_MS - used) / 1000UL;
}

void fanCtrlSetEnabled(bool enabled) {
  s_enabled = enabled;
  if (enabled) {
    // When re-enabling AUTO, reset tick base so we don't jump a huge dt
    s_lastTick = millis();
  }
}
