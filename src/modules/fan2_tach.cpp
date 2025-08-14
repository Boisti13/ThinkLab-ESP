#include "fan2_tach.h"

#if USE_FAN2_TACH
static volatile uint32_t s2_pulses     = 0;
static volatile uint32_t s2_lastEdgeUs = 0;
static volatile uint32_t s2_lastPulseMs= 0;

static int      s2_rpm        = -1;
static uint32_t s2_lastCalcMs = 0;
static uint32_t s2_lastCount  = 0;

static inline bool acceptEdge2(uint32_t nowUs) {
  uint32_t dt = nowUs - s2_lastEdgeUs;
  return dt > 500;
}

static void IRAM_ATTR onTach2Edge() {
  uint32_t nowUs = micros();
  if (!acceptEdge2(nowUs)) return;
  s2_lastEdgeUs  = nowUs;
  s2_pulses++;
  s2_lastPulseMs = millis();
}

void fan2TachBegin() {
  pinMode(FAN2_TACH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FAN2_TACH_PIN), onTach2Edge, FALLING);
  s2_pulses = 0;
  s2_lastEdgeUs = 0;
  s2_lastPulseMs = 0;
  s2_rpm = -1;
  s2_lastCalcMs = millis();
  s2_lastCount  = 0;
}

void fan2TachTick() {
  const uint32_t now = millis();
  const uint32_t win = FAN2_TACH_WIN_MS;
  if (now - s2_lastCalcMs < win) return;

  uint32_t cnt, lastPulseMs;
  noInterrupts();
  cnt = s2_pulses;
  lastPulseMs = s2_lastPulseMs;
  interrupts();

  uint32_t delta = cnt - s2_lastCount;
  uint32_t dtMs  = now - s2_lastCalcMs;
  s2_lastCount   = cnt;
  s2_lastCalcMs  = now;

  if (delta > 0 && dtMs > 0) {
    float rpmF = (float)delta * (60000.0f / (float)dtMs) / (float)FAN2_PPR;
    s2_rpm = (int)(rpmF + 0.5f);
  } else {
    if (now - lastPulseMs > FAN2_TACH_TIMEOUT_MS) s2_rpm = 0;
  }
}

int fan2TachGetRPM() { return s2_rpm; }
#endif
