#include "page_disks.h"
#include "ui_theme.h"
#include "state.h"
#include <Fonts/FreeMono9pt7b.h>

// Measure width with current font
static uint16_t textW(Epd_t& d, const String& s) {
  int16_t x1, y1; uint16_t w, h;
  d.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  return w;
}

// Draw compact status icon at the right edge.
// Active = inner fill; Idle/other = outline only.
static void drawStatusIcon(Epd_t& d, int16_t xRight, int16_t baselineY, bool active) {
  const int16_t W = 8, H = 8;
  const int16_t x = xRight - W;
  const int16_t y = baselineY - 7; // centers for 9pt font baseline
  d.drawRect(x, y, W, H, GxEPD_BLACK);
  if (active) d.fillRect(x + 2, y + 2, W - 4, H - 4, GxEPD_BLACK);
}

void PageDisks::render(Epd_t& d, const HostState& host, const UiState& /*ui*/) {
  d.firstPage();
  do {
    d.fillScreen(GxEPD_WHITE);

    ui::header(d, F("Disks"));
    d.setFont(&FreeMono9pt7b);

    // Layout knobs (LOCAL to this page)
    const int16_t LINE_H   = 20;
    const int16_t bulletX  = 0;               // dash bullet all the way left
    const int16_t valueR   = d.width() - 4;   // right edge for values/icons
    const int16_t gapMin   = 6;               // gap between temp and icon
    const int16_t ICON_W   = 8;               // must match drawStatusIcon()
    const int16_t TEMP_SHIFT_LEFT = 10;       // <— move temp this many px left

    // Name starts right after "- "
    const uint16_t dashW   = textW(d, String(F("- ")));
    const int16_t  nameX   = bulletX + dashW;

    // Reserve space on right: icon, gap, then temp (shifted left)
    const int16_t tempValueR = valueR - (ICON_W + gapMin + 1 + TEMP_SHIFT_LEFT);

    int16_t y = ui::content_top();

    // Fit calculation
    const int16_t rowsAvailPx = (int16_t)(d.height() - y);
    const int16_t maxRows     = rowsAvailPx / LINE_H;
    const uint8_t showN       = (host.disk_count < maxRows) ? host.disk_count : (uint8_t)maxRows;

    if (showN == 0) {
      d.setCursor(4, y);
      d.print(F("No disks"));
    } else {
      for (uint8_t i = 0; i < showN; ++i) {
        const DiskInfo &dk = host.disks[i];

        // Left: "- " + name
        d.setCursor(bulletX, y); d.print(F("- "));
        d.setCursor(nameX,   y); d.print(dk.name[0] ? dk.name : "-");

        // Rightmost: status icon
        drawStatusIcon(d, valueR, y, dk.active);

        // Temperature right-aligned just to the left of the icon (with extra left shift)
        String t = (dk.temp_c <= -100) ? String("-") : String(dk.temp_c) + "C";
        ui::printRight(d, tempValueR, y, t);

        y += LINE_H;
      }
    }

    // no footer

  } while (d.nextPage());
}
