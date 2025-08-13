#include "pages/page_debug.h"
#include "ui_theme.h"
#include "config.h"
#include "state.h"
#include <Fonts/FreeMono9pt7b.h>

void PageDebug::render(Epd_t& d, const HostState& /*host*/, const UiState& ui) {
  d.firstPage();
  do {
    d.fillScreen(GxEPD_WHITE);

    ui::header(d, F("Debug"));
    d.setFont(&FreeMono9pt7b);

    const int16_t LINE_H = 20;
    const int16_t labelX = 4;
    const int16_t valueR = d.width() - 4;

    int16_t y = ui::content_top();

    // FW_Version
    d.setCursor(labelX, y); d.print(F("FW:"));
    ui::printRight(d, valueR, y, String(FW_VERSION));
    y += LINE_H;

    // RX age
    d.setCursor(labelX, y); d.print(F("RX age:"));
    ui::printRight(d, valueR, y, ui.lastParseOkMs ? String(secsSince(ui.lastParseOkMs)) + "s" : String("—"));
    y += LINE_H;

    // OK/ERR
    d.setCursor(labelX, y); d.print(F("OK/ERR:"));
    ui::printRight(d, valueR, y, String(ui.parseOkCount) + "/" + String(ui.parseErrCount));
    y += LINE_H;

    // JSON len
    d.setCursor(labelX, y); d.print(F("JSON len:"));
    ui::printRight(d, valueR, y, String(ui.lastJsonLen) + " B");
    y += LINE_H;

    // Poll (s)
    d.setCursor(labelX, y); d.print(F("Poll:"));
    ui::printRight(d, valueR, y, String(POLL_INTERVAL_MS / 1000.0f, 1) + "s");
    y += LINE_H;

    // Current Mode
    d.setCursor(labelX, y); d.print(F("Mode:"));
    ui::printRight(d, valueR, y, ui.mode == MODE_TOUCH ? String("TOUCH") : String("AUTO"));
    y += LINE_H;

    // Debug ON/OFF
    d.setCursor(labelX, y); d.print(F("Debug:"));
    ui::printRight(d, valueR, y, ui.debugEnabled ? String("ON") : String("OFF"));

  } while (d.nextPage());
}
