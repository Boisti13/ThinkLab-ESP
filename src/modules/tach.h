#pragma once
#include <Arduino.h>

// Fan tachometer measured by the time between pulses, not by counting pulses
// in a window: at low speed a 500 ms window holds only a few pulses, which
// made 200 rpm read as 180, 240 or 300. The speed is taken from the median of
// the last TACH_AVG_PERIODS periods, so one missed or extra pulse (the tach
// line is noisy at low PWM duty) doesn't move it.
#ifndef TACH_AVG_PERIODS
#define TACH_AVG_PERIODS 6
#endif
#ifndef TACH_MIN_PERIOD_US
#define TACH_MIN_PERIOD_US 1500   // shorter = glitch (would be > 20000 rpm at 2 PPR)
#endif

class Tach {
public:
  void begin(uint8_t pin, uint8_t ppr, uint32_t timeoutMs);
  void tick();                 // call in loop(); updates rpm() every 250 ms
  int  rpm() const { return rpm_; }   // -1 = not measured yet, 0 = stopped

private:
  static void IRAM_ATTR onEdge(void* self);

  uint8_t  pin_ = 0;
  uint8_t  ppr_ = 2;
  uint32_t timeoutUs_ = 2000000;
  volatile uint32_t lastEdgeUs_ = 0;
  volatile uint32_t periods_[TACH_AVG_PERIODS] = {0};
  volatile uint8_t  idx_ = 0;
  volatile uint8_t  filled_ = 0;   // valid periods in periods_[]
  volatile bool     seenEdge_ = false;
  volatile uint32_t lastPeriodUs_ = 0;   // last accepted period, for the noise filter
  uint32_t lastCalcMs_ = 0;
  uint32_t beginMs_ = 0;
  int      rpm_ = -1;
};
