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
