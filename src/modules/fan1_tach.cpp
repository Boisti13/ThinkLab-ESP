#include "fan1_tach.h"

#if USE_FAN1
static volatile uint32_t s_pulses = 0;
static volatile uint32_t s_lastEdgeUs = 0;
static volatile uint32_t s_lastPulseMs = 0;

static int      s_rpm        = -1;
static uint32_t s_lastCalcMs = 0;
static uint32_t s_lastCount  = 0;

static inline bool acceptEdge(uint32_t nowUs) {
  uint32_t dt = nowUs - s_lastEdgeUs;
  return dt > 500; // 500 µs debounce
}

static void IRAM_ATTR onTachEdge() {
  uint32_t nowUs = micros();
  if (!acceptEdge(nowUs)) return;
  s_lastEdgeUs  = nowUs;
  s_pulses++;
  s_lastPulseMs = millis();
}

void fan1TachBegin() {
  pinMode(FAN1_TACH_PIN, INPUT_PULLUP); // open-collector tach → pull up to 3.3V
  attachInterrupt(digitalPinToInterrupt(FAN1_TACH_PIN), onTachEdge, FALLING);
  s_pulses = 0;
  s_lastEdgeUs = 0;
  s_lastPulseMs = 0;
  s_rpm = -1;
  s_lastCalcMs = millis();
  s_lastCount  = 0;
}

void fan1TachTick() {
  const uint32_t now = millis();
  const uint32_t win = FAN1_TACH_WIN_MS;
  if (now - s_lastCalcMs < win) return;

  uint32_t cnt, lastPulseMs;
  noInterrupts();
  cnt = s_pulses;
  lastPulseMs = s_lastPulseMs;
  interrupts();

  uint32_t delta = cnt - s_lastCount;
  uint32_t dtMs  = now - s_lastCalcMs;
  s_lastCount    = cnt;
  s_lastCalcMs   = now;

  if (delta > 0 && dtMs > 0) {
    float rpmF = (float)delta * (60000.0f / (float)dtMs) / (float)FAN1_PPR;
    s_rpm = (int)(rpmF + 0.5f);
  } else {
    if (now - lastPulseMs > FAN1_TACH_TIMEOUT_MS) {
      s_rpm = 0;
    }
  }
}

int fan1TachGetRPM() { return s_rpm; }
#endif
