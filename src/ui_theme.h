#pragma once
#include "display_manager.h"
#include <GxEPD2_BW.h>

namespace ui {
  // Compact header metrics
  static constexpr int16_t HEADER_H    = 24;   // header band height
  static constexpr int16_t MARGIN_X    = 4;
  static constexpr int16_t TITLE_BASE  = 16;   // baseline for bold title
  static constexpr int16_t CONTENT_PAD = 20;   // gap below the underline

  inline int16_t content_top() { return HEADER_H + CONTENT_PAD; }

  // Draw centered bold header + 1px underline
  void header(Epd_t& d, const __FlashStringHelper* title);

  // Right-aligned text at xRight (with current font)
  inline void printRight(Epd_t& d, int16_t xRight, int16_t y, const String& s) {
    int16_t x1, y1; uint16_t w, h;
    d.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    int16_t x = xRight - (int16_t)w;
    if (x < 0) x = 0;
    d.setCursor(x, y);
    d.print(s);
  }
}
