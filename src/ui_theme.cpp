#include "ui_theme.h"
#include <Fonts/FreeMonoBold9pt7b.h>
#include <WiFi.h>

// =======================
// Header (centered title)
// =======================
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

  d.drawFastHLine(0, HEADER_H - 1, W, GxEPD_BLACK);
}

// =======================
// Wi-Fi helpers
// =======================
int ui::rssi_to_bars(int16_t rssi) {
  // Treat invalid RSSI values as 0 bars
  if (rssi == 0 || rssi == -1 || rssi == 31) return 0;

  if (rssi > -55)      return 4; // excellent
  else if (rssi > -65) return 3; // good
  else if (rssi > -75) return 2; // fair
  else if (rssi > -85) return 1; // weak
  else                 return 0; // none
}

void ui::wifi_icon(Epd_t& d, int16_t cx, int16_t cy, int bars) {
  // Clamp bars to [0..4]
  if (bars < 0) bars = 0;
  if (bars > 4) bars = 4;

  // Bar geometry
  const int barWidth = 4;     // thicker so "empty" vs "filled" is visible
  const int spacing  = 3;     // gap between bars
  // Height formula: (i+1)*2 + 4  -> tallest (i=3) = 12 px
  auto barHeight = [](int i){ return (i + 1) * 2 + 4; };

  // Center horizontally on cx
  const int totalWidth = 4 * barWidth + 3 * spacing; // 25 px with 4/3 above
  const int x0 = cx - totalWidth / 2;
  const int baseY = cy;  // treat cy as baseline (same y you used before)

  for (int i = 0; i < 4; i++) {
    const int h = barHeight(i);
    const int x = x0 + i * (barWidth + spacing);
    if (i < bars) {
      d.fillRect(x, baseY - h, barWidth, h, GxEPD_BLACK);
    } else {
      // draw hollow bar so it's clearly "empty" on e-ink
      d.drawRect(x, baseY - h, barWidth, h, GxEPD_BLACK);
    }
  }
}


// =======================
// Optional global footer
// =======================
void ui::footer(Epd_t& d, bool /*show_ip*/, bool icon_right) {
  const int16_t W = d.width();
  const int16_t H = d.height();

  // Icon center a few px above bottom so arcs fit
  const int16_t cy = H - 6;
  const int16_t iconHalf = WIFI_OUTER_R;

  const bool connected = (WiFi.status() == WL_CONNECTED);
  const int16_t rssi = connected ? WiFi.RSSI() : -999;
  const int bars = connected ? rssi_to_bars(rssi) : 0;

  // Icon X position (right or left)
  const int16_t cx = icon_right
                     ? (W - ui::MARGIN_X - iconHalf)
                     : (ui::MARGIN_X + iconHalf);

  // Draw the icon only (no IP text)
  wifi_icon(d, cx, cy - 2, bars);
}

