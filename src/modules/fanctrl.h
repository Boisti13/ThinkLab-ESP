#pragma once
#include <Arduino.h>
#include "config.h"
#include "state.h"
#include "fan1_pwm.h"

void fanCtrlBegin();
void fanCtrlTick(HostState& host);
