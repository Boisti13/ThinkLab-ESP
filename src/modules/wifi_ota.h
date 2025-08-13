#pragma once
#include "config.h"
#if USE_WIFI
class WifiOta {
public:
  void begin();
  void tick();
};
#endif
