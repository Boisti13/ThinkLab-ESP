#include "tach.h"

void IRAM_ATTR Tach::onEdge(void* arg) {
  Tach* t = static_cast<Tach*>(arg);
  // A real tach pulse holds the line low for milliseconds; PWM switching
  // couples spikes of a few microseconds into it, gone by the time we look.
  if (digitalRead(t->pin_)) return;
  const uint32_t now = micros();
  const uint32_t dt = now - t->lastEdgeUs_;
  if (t->seenEdge_) {
    if (dt < TACH_MIN_PERIOD_US) return;   // glitch, keep the previous edge
    // A pulse after less than half the last period is noise on the tach line
    // (a fan can't double its speed from one pulse to the next).
    if (t->lastPeriodUs_ && dt < t->lastPeriodUs_ / 2) return;
    if (dt <= t->timeoutUs_) {
      t->lastPeriodUs_ = dt;
      t->periods_[t->idx_] = dt;
      t->idx_ = (t->idx_ + 1) % TACH_AVG_PERIODS;
      if (t->filled_ < TACH_AVG_PERIODS) t->filled_++;
    } else {
      t->filled_ = 0;                      // first pulse after a stop: start over
      t->lastPeriodUs_ = 0;
    }
  }
  t->lastEdgeUs_ = now;
  t->seenEdge_ = true;
}

void Tach::begin(uint8_t pin, uint8_t ppr, uint32_t timeoutMs) {
  pin_ = pin;
  ppr_ = ppr ? ppr : 2;
  timeoutUs_ = timeoutMs * 1000UL;
  beginMs_ = millis();
  pinMode(pin, INPUT_PULLUP);            // open-collector tach, pull-up to 3.3 V
  attachInterruptArg(digitalPinToInterrupt(pin), Tach::onEdge, this, FALLING);
}

void Tach::tick() {
  const uint32_t nowMs = millis();
  if (nowMs - lastCalcMs_ < 250) return;
  lastCalcMs_ = nowMs;

  uint32_t lastEdge, p[TACH_AVG_PERIODS];
  uint8_t n;
  bool seen;
  noInterrupts();
  lastEdge = lastEdgeUs_;
  n = filled_;
  seen = seenEdge_;
  for (uint8_t i = 0; i < n; ++i) p[i] = periods_[i];
  interrupts();

  if (!seen || micros() - lastEdge > timeoutUs_) {
    // no pulse for a while: stopped (or not measured yet right after boot)
    rpm_ = (nowMs - beginMs_ > timeoutUs_ / 1000UL) ? 0 : -1;
    return;
  }
  if (n == 0) return;                    // only one pulse so far: keep the last value

  // Median period (mean of the middle two for an even count)
  for (uint8_t i = 1; i < n; ++i) {
    const uint32_t v = p[i];
    int8_t j = i - 1;
    while (j >= 0 && p[j] > v) { p[j + 1] = p[j]; --j; }
    p[j + 1] = v;
  }
  const float period = (n % 2) ? (float)p[n / 2] : ((float)p[n / 2 - 1] + (float)p[n / 2]) / 2.0f;
  if (period <= 0.0f) return;
  const float rpm = 60.0e6f / (period * ppr_);
  rpm_ = (int)(rpm + 0.5f);
}
