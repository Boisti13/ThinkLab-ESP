#pragma once
#include "display_manager.h"
#include <GxEPD2_BW.h>

namespace ui {
  // --- Layout metrics ---
  static constexpr int16_t HEADER_H    = 24;   // header band height
  static constexpr int16_t MARGIN_X    = 4;
  static constexpr int16_t TITLE_BASE  = 16;   // baseline for bold title
  static constexpr int16_t CONTENT_PAD = 20;   // gap below the underline

  inline int16_t content_top() { return HEADER_H + CONTENT_PAD; }

  // --- Header ---
  // Draw centered bold header + 1px underline
  void header(Epd_t& d, const __FlashStringHelper* title);

  // --- Text helpers ---
  // Right-aligned text at xRight (with current font)
  inline void printRight(Epd_t& d, int16_t xRight, int16_t y, const String& s) {
    int16_t x1, y1; uint16_t w, h;
    d.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    int16_t x = xRight - (int16_t)w;
    if (x < 0) x = 0;
    d.setCursor(x, y);
    d.print(s);
  }

  // --- Wi-Fi icon metrics ---
  static constexpr int16_t WIFI_OUTER_R = 11;  // outer arc radius
  static constexpr int16_t WIFI_DOT_R   = 2;   // dot radius

  // --- Wi-Fi helpers ---
  // Map RSSI (dBm) → 0..3 bars
  int rssi_to_bars(int16_t rssi);

  // Draw compact Wi-Fi icon at (cx,cy) with given bar count
  void wifi_icon(Epd_t& d, int16_t cx, int16_t cy, int bars);

  // (optional) Global footer with Wi-Fi symbol + IP
  void footer(Epd_t& d, bool show_ip = true, bool icon_right = true);
}
