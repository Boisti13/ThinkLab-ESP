#pragma once
#include "config.h"
#if USE_FANCTRL
class FanController {
public:
  void begin();
  void tick();  // future: PID/logic
  void setDuty(uint8_t pct);
private:
  uint8_t duty_ = 0;
};
#endif
