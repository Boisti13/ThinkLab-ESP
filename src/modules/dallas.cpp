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
      lastC_ = dallas.getTempCByIndex(0);
      lastSampleMs = now;
      state = IDLE;
    }
  }
}
