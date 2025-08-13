#include "page_vms.h"
#include "ui_theme.h"
#include "state.h"
#include <Fonts/FreeMono9pt7b.h>

// Measure width with current font
static uint16_t textW(Epd_t& d, const String& s) {
  int16_t x1, y1; uint16_t w, h;
  d.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  return w;
}

// Truncate to fit width (adds "..." if needed)
static String fitToWidth(Epd_t& d, const String& s, int16_t maxW) {
  int16_t x1, y1; uint16_t w, h;
  d.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  if (w <= maxW) return s;

  String base = s;
  while (base.length() > 1) {
    base.remove(base.length() - 1);
    String trial = base + "...";
    d.getTextBounds(trial, 0, 0, &x1, &y1, &w, &h);
    if (w <= maxW) return trial;
  }
  return String("...");
}

// Clamp helper to avoid std::max<int16_t>(...)
static inline int16_t nonneg(int16_t v) { return v < 0 ? 0 : v; }

// Draw a compact status icon at the right edge.
// Running = inner fill; Stopped = outline only.
static void drawStatusIcon(Epd_t& d, int16_t xRight, int16_t baselineY, bool running) {
  const int16_t W = 8, H = 8;
  const int16_t x = xRight - W;
  const int16_t y = baselineY - 7; // centers vs baseline for 9pt font
  d.drawRect(x, y, W, H, GxEPD_BLACK);
  if (running) {
    d.fillRect(x + 2, y + 2, W - 4, H - 4, GxEPD_BLACK);
  }
}

void PageVMs::render(Epd_t& d, const HostState& host, const UiState& /*ui*/) {
  d.firstPage();
  do {
    d.fillScreen(GxEPD_WHITE);

    ui::header(d, F("VMs / LXCs"));
    d.setFont(&FreeMono9pt7b);

    // Layout knobs (LOCAL to this page)
    const int16_t LINE_H   = 20;
    const int16_t labelX   = 4;               // for "VMs:" / "LXCs:"
    const int16_t valueR   = d.width() - 4;   // right edge for values/icons
    const int16_t bulletX  = 0;               // dash bullet all the way left
    const int16_t gapMin   = 6;               // gap between name and icon
    const int16_t ICON_W   = 8;               // status icon width (must match drawStatusIcon)

    // Where names start (after "- ")
    const uint16_t dashW = textW(d, String(F("- ")));
    const int16_t  nameStartX = bulletX + dashW;

    int16_t y = ui::content_top();

    // ---- VMs: running/total ----
    d.setCursor(labelX, y); d.print(F("VMs:"));
    {
      String v = (host.vms_running >= 0 && host.vms_total >= 0)
                 ? String(host.vms_running) + "/" + String(host.vms_total)
                 : String("-");
      ui::printRight(d, valueR, y, v);
    }
    y += LINE_H;

    // ---- VM list (names left, icon right) ----
    {
      const uint8_t SHOW_N = 2;
      const uint8_t n = (host.vm_list_count < SHOW_N) ? host.vm_list_count : SHOW_N;
      for (uint8_t i = 0; i < n; ++i) {
        // Reserve space on right for icon + gap
        const int16_t nameMaxW = (valueR - gapMin - ICON_W) - nameStartX;

        d.setCursor(bulletX, y); d.print(F("- "));
        String nm = fitToWidth(d, String(host.vm_list[i].name), nonneg(nameMaxW));
        d.setCursor(nameStartX, y); d.print(nm);

        drawStatusIcon(d, valueR, y, host.vm_list[i].running);
        y += LINE_H;
      }
    }

    // ---- LXCs: running/total ----
    d.setCursor(labelX, y); d.print(F("LXCs:"));
    {
      String v = (host.lxcs_running >= 0 && host.lxcs_total >= 0)
                 ? String(host.lxcs_running) + "/" + String(host.lxcs_total)
                 : String("-");
      ui::printRight(d, valueR, y, v);
    }
    y += LINE_H;

    // ---- LXC list (names left, icon right) ----
    {
      const uint8_t SHOW_N = 2;
      const uint8_t n = (host.lxc_list_count < SHOW_N) ? host.lxc_list_count : SHOW_N;
      for (uint8_t i = 0; i < n; ++i) {
        const int16_t nameMaxW = (valueR - gapMin - ICON_W) - nameStartX;

        d.setCursor(bulletX, y); d.print(F("- "));
        String nm = fitToWidth(d, String(host.lxc_list[i].name), nonneg(nameMaxW));
        d.setCursor(nameStartX, y); d.print(nm);

        drawStatusIcon(d, valueR, y, host.lxc_list[i].running);
        y += LINE_H;
      }
    }

    // no footer

  } while (d.nextPage());
}
