#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"
#include "modules/dallas.h"

static OneWire oneWire(PIN_ONEWIRE);
static DallasTemperature dallas(&oneWire);

void DallasProbe::begin() {
  dallas.begin();
  dallas.setResolution(10);            // 10-bit ~187ms (good compromise)
  dallas.setWaitForConversion(false);  // non-blocking
}

void DallasProbe::tick() {
  uint32_t now = millis();

  // sample every 10s
  if (state == IDLE) {
    if (now - lastSampleMs >= 10000) {
      dallas.requestTemperatures();    // start conversion (returns immediately)
      tStart = now;
      state = CONVERTING;
    }
    return;
  }

  // CONVERTING -> read after typical conversion time
  if (state == CONVERTING) {
    if (now - tStart >= 200) {         // >187ms for 10-bit
      // The library reports errors as numbers: -127 (DEVICE_DISCONNECTED_C)
      // when the sensor doesn't answer, 85.0 = power-on value of a reading
      // that never ran. Both mean "no reading", not a temperature.
      const float t = dallas.getTempCByIndex(0);
      lastC_ = (t <= DEVICE_DISCONNECTED_C + 1.0f || t == 85.0f || t > 125.0f) ? NAN : t;
      lastSampleMs = now;
      state = IDLE;
    }
  }
}
