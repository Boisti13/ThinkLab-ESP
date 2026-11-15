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

// Stall protection (both fans share the PWM line): a fan that has turned
// since boot but reads 0 rpm for FAN_STALL_MS while driven gets a kick; after
// FAN_STALL_RAISE_AFTER failed kicks the minimum duty goes up by
// FAN_MIN_DUTY_STEP (up to FAN_MIN_DUTY_MAX, until the next restart).
uint8_t  fanCtrlMinDuty();       // current minimum duty, %
bool     fanCtrlStalled();       // a fan is stopped although it should run
uint32_t fanCtrlStallKicks();    // stall kicks since boot
