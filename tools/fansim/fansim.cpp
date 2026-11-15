// Runs src/modules/fanctrl.cpp on a PC against scripted case temperatures
// and prints what the fan would do (see README.md, Development).
#include <cstdio>
#include <cmath>
#include "state.h"
#include "modules/fanctrl.h"

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

struct Step { float tempC; int seconds; const char *what; };

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
      {NAN, 60, "sensor unplugged (dallas.cpp: NaN)"},
      {31.0f, 900, "back, cooling to 31 C"},
  };
  for (const Step &s : plan) {
    const int kicks0 = g_kicks, sw0 = g_switches;
    int onMs = 0;
    for (int t = 0; t < s.seconds * 100; ++t) {
      g_ms += 10;
      h.local_temp_c = s.tempC;
      fanCtrlTick(h);
      if (g_pwm > 0) onMs += 10;
    }
    printf("%-38s -> fan %3u %%  (on %3d %% of the time, %d switches, %d kicks)\n", s.what, g_pwm,
           onMs * 100 / (s.seconds * 1000), g_switches - sw0, g_kicks - kicks0);
  }
  return 0;
}
