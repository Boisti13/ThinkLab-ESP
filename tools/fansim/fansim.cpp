// Runs src/modules/fanctrl.cpp on a PC against scripted case temperatures
// and a simple model of the two fans on the shared PWM line, and prints what
// the fans would do (see README.md, Development).
#include <cmath>
#include <cstdio>

#include "modules/fanctrl.h"
#include "state.h"

static uint32_t g_ms = 0;
uint32_t millis() { return g_ms; }
static uint8_t g_pwm = 0;
static int g_kicks = 0, g_switches = 0;
void fan1PwmBegin() {}
void fan1PwmSetPercent(uint8_t p) {
  if (p > 0 && p < FAN1_MIN_DUTY_PCT) p = FAN1_MIN_DUTY_PCT;
  if (p == 100 && g_pwm != 100) g_kicks++;
  if ((p > 0) != (g_pwm > 0)) g_switches++;
  g_pwm = p;
}
uint8_t fan1PwmGetPercent() { return g_pwm; }

// Fan model: it keeps turning down to stallBelow %, but once stopped needs
// startAt % (or a kick) to get going again. connected=false: no tach at all.
struct Fan {
  int stallBelow, startAt;
  bool connected, turning;
  int rpm() const { return !connected ? -1 : (turning ? g_pwm * 25 : 0); }
  void step() {
    if (!connected) return;
    if (turning && g_pwm < stallBelow) turning = false;
    if (!turning && g_pwm >= startAt) turning = true;
  }
};
static Fan g_fan[2] = {{5, 20, true, false}, {5, 20, true, false}};
int fan1TachGetRPM() { return g_fan[0].rpm(); }
int fan2TachGetRPM() { return g_fan[1].rpm(); }

struct Step {
  float tempC;
  int seconds;
  const char *what;
  int fan2StallBelow = -1;  // change fan 2's stall point from here on
};

int main() {
  static HostState h;
  fanCtrlBegin();
  // The board's loop calls fanCtrlTick about every 10 ms; e-paper redraws
  // stall it, which this run leaves out (worst case for the controller).
  const Step plan[] = {
      {NAN, 20, "boot, sensor not read yet"},
      {34.0f, 300, "normal case 34 C"},
      {30.0f, 600, "cool night 30 C"},
      {33.0f, 600, "warmer 33 C"},
      {36.0f, 600, "36 C"},
      {40.0f, 600, "40 C"},
      {32.0f, 1800, "hovering at the 32 C edge"},
      {32.0f, 600, "fan 2 worn: stalls below 11 %", 11},
      {NAN, 60, "sensor unplugged (dallas.cpp: NaN)"},
      {31.0f, 900, "back, cooling to 31 C"},
  };
  for (const Step &s : plan) {
    if (s.fan2StallBelow >= 0) g_fan[1].stallBelow = s.fan2StallBelow;
    const int kicks0 = g_kicks, sw0 = g_switches;
    int onMs = 0, stopped2Ms = 0;
    for (int t = 0; t < s.seconds * 100; ++t) {
      g_ms += 10;
      h.local_temp_c = s.tempC;
      fanCtrlTick(h);
      for (Fan &f : g_fan) f.step();
      if (g_pwm > 0) onMs += 10;
      if (g_pwm > 0 && g_fan[1].rpm() == 0) stopped2Ms += 10;
    }
    printf("%-36s -> duty %3u %% min %2u %%  on %3d %%  fan2 stopped %3d %%  %d switches %d kicks\n",
           s.what, g_pwm, fanCtrlMinDuty(), onMs * 100 / (s.seconds * 1000),
           stopped2Ms * 100 / (s.seconds * 1000), g_switches - sw0, g_kicks - kicks0);
  }
  return 0;
}
