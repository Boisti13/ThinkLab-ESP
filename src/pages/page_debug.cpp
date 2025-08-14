#include "page_debug.h"
#include "ui_theme.h"
#include "state.h"
#include "config.h"
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/Picopixel.h>

#if USE_FAN1
  #include "modules/fan1_pwm.h"
  #include "modules/fan1_tach.h"
#endif
#if USE_FAN2_TACH
  #include "modules/fan2_tach.h"
#endif



void PageDebug::render(Epd_t& d, const HostState& host, const UiState& ui) {
  d.firstPage();
  do {
    d.fillScreen(GxEPD_WHITE);

    ui::header(d, F("Debug"));
    d.setFont(&FreeMono9pt7b);
    //d.setFont(&Picopixel);

    // layout knobs (LOCAL)
    const int16_t LINE_H = 15;
    const int16_t labelX = 4;
    const int16_t valueR = d.width() - 4;
    int16_t y = ui::content_top();

    // FW version
    d.setCursor(labelX, y); d.print(F("FW:"));
    ui::printRight(d, valueR, y, String(FW_VERSION));
    y += LINE_H;

    // RX age
    d.setCursor(labelX, y); d.print(F("RX age:"));
    ui::printRight(d, valueR, y, ui.lastParseOkMs ? String(secsSince(ui.lastParseOkMs)) + "s" : String("-"));
    y += LINE_H;

    // OK/ERR
    d.setCursor(labelX, y); d.print(F("OK/ERR:"));
    ui::printRight(d, valueR, y, String(ui.parseOkCount) + "/" + String(ui.parseErrCount));
    y += LINE_H;

    // JSON len
    d.setCursor(labelX, y); d.print(F("JSON len:"));
    ui::printRight(d, valueR, y, String(ui.lastJsonLen) + " B");
    y += LINE_H;

    // Poll
    d.setCursor(labelX, y); d.print(F("Poll:"));
    ui::printRight(d, valueR, y, String(POLL_INTERVAL_MS / 1000.0f, 1) + "s");
    y += LINE_H;

    // Mode
    d.setCursor(labelX, y); d.print(F("Mode:"));
    ui::printRight(d, valueR, y, ui.mode == MODE_TOUCH ? String("TOUCH") : String("AUTO"));
    y += LINE_H;

    // Debug enabled
    //d.setCursor(labelX, y); d.print(F("Debug:"));
    //ui::printRight(d, valueR, y, ui.debugEnabled ? String("ON") : String("OFF"));
    //y += LINE_H;

    // --- ADD: Fan PWM / RPM (right-aligned) ---
#if USE_FAN1
  d.setCursor(labelX, y); d.print(F("Fan1 PWM:"));
  ui::printRight(d, valueR, y, String(fan1PwmGetPercent()) + "%");
  y += LINE_H;

  d.setCursor(labelX, y); d.print(F("Fan1 RPM:"));
  {
    int rpm1 = fan1TachGetRPM();
    ui::printRight(d, valueR, y, rpm1 < 0 ? String("—") : String(rpm1));
  }
  y += LINE_H;
#endif

#if USE_FAN2_TACH
  d.setCursor(labelX, y); d.print(F("Fan2 RPM:"));
  {
    int rpm2 = fan2TachGetRPM();
    ui::printRight(d, valueR, y, rpm2 < 0 ? String("—") : String(rpm2));
  }
  y += LINE_H;
#endif

  } while (d.nextPage());
}
