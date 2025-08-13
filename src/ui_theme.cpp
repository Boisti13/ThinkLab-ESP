#include "ui_theme.h"
#include <Fonts/FreeMonoBold9pt7b.h>

void ui::header(Epd_t& d, const __FlashStringHelper* title) {
  const int16_t W = d.width();
  d.fillRect(0, 0, W, HEADER_H, GxEPD_WHITE);

  d.setTextColor(GxEPD_BLACK);
  d.setFont(&FreeMonoBold9pt7b);

  String t(title);
  int16_t x1, y1; uint16_t w, h;
  d.getTextBounds(t, 0, TITLE_BASE, &x1, &y1, &w, &h);
  int16_t x = (W - (int16_t)w) / 2; if (x < 0) x = 0;
  d.setCursor(x, TITLE_BASE);
  d.print(t);

  d.drawFastHLine(0, HEADER_H - 1, W, GxEPD_BLACK); // <— one pixel lower
}
