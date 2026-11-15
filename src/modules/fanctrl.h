#pragma once
#include <Arduino.h>
#include "config.h"
#include "state.h"
#include "modules/fan1_pwm.h"   // match repo path

// Initialise fan controller
void fanCtrlBegin();

// Periodic tick, called from loop()
void fanCtrlTick(HostState& host);

// Enable/disable automatic fan control (used by web_server for manual override)
bool fanCtrlGetEnabled();
void fanCtrlSetEnabled(bool enabled);

// Manual duty from the web page. Ends by itself after FAN_MANUAL_MAX_MS or
// when the case reaches FAN_MANUAL_OVERRIDE_C; then the curve takes over.
void fanCtrlSetManual(uint8_t pct);
uint32_t fanCtrlManualLeftS();   // 0 in AUTO
