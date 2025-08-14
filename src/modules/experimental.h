#pragma once
#include <Arduino.h>
#include "config.h"
#include "state.h"

#if USE_EXPERIMENTAL
  void experimentalBegin();
  void experimentalTick(UiState& ui);
#else
  inline void experimentalBegin() {}
  inline void experimentalTick(UiState&) {}
#endif
