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

// Render a list with a strict cap: show up to (cap) lines.
// If count > cap: render (cap-1) real entries and then a "+N more" line.
// Returns lines consumed.
template <typename ItemT>
static uint8_t renderListWithStrictCap(
  Epd_t& d,
  const ItemT* list,
  uint8_t count,
  int16_t& y,
  const int16_t bulletX,
  const int16_t nameStartX,
  const int16_t valueR,
  const int16_t gapMin,
  const int16_t ICON_W,
  const int16_t LINE_H
) {
  if (count == 0) return 0;

  const uint8_t cap = (uint8_t)((void)ICON_W, 0); // placeholder to silence unused warning in generic template
  // We'll pass the cap via a lambda wrapper below. This template keeps logic generic.
  return 0;
}

// Wrapper with explicit cap value to avoid template parameter clutter
template <typename ItemT>
static uint8_t renderListWithStrictCapCap(
  Epd_t& d,
  const ItemT* list,
  uint8_t count,
  uint8_t cap,
  int16_t& y,
  const int16_t bulletX,
  const int16_t nameStartX,
  const int16_t valueR,
  const int16_t gapMin,
  const int16_t ICON_W,
  const int16_t LINE_H
) {
  if (count == 0 || cap == 0) return 0;

  const uint8_t lines = (count <= cap) ? count : cap;
  const bool needsMoreLine = (count > cap);

  // When we need a "+N more", we render only (cap - 1) real items.
  const uint8_t realItems = needsMoreLine ? (uint8_t)(cap - 1) : lines;

  // Space available for the name text (reserve right side for icon + gap)
  const int16_t nameMaxW = nonneg((valueR - gapMin - ICON_W) - nameStartX);

  uint8_t consumed = 0;

  // Real items
  for (uint8_t i = 0; i < realItems; ++i) {
    d.setCursor(bulletX, y); d.print(F("- "));
    String nm = fitToWidth(d, String(list[i].name), nameMaxW);
    d.setCursor(nameStartX, y); d.print(nm);
    drawStatusIcon(d, valueR, y, list[i].running);
    y += LINE_H;
    consumed++;
  }

  // "+N more" line (no icon)
  if (needsMoreLine) {
    const uint8_t remaining = (uint8_t)(count - realItems);
    d.setCursor(nameStartX, y);
    d.print(String(F("+")) + String(remaining) + F(" more"));
    y += LINE_H;
    consumed++;
  }

  return consumed;
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
    const int16_t valueR   = d.width() - 4;   // right edge for values/icons (right-aligned)
    const int16_t bulletX  = 0;               // dash bullet all the way left
    const int16_t gapMin   = 6;               // gap between name and icon
    const int16_t ICON_W   = 8;               // status icon width (must match drawStatusIcon)

    // Where names start (after "- ")
    const uint16_t dashW = textW(d, String(F("- ")));
    const int16_t  nameStartX = bulletX + dashW;

    // Section caps: total 7 names on screen -> 3 VMs + 4 LXCs
    const uint8_t VM_CAP  = 3;
    const uint8_t LXC_CAP = 4;

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

    // ---- VM list (up to VM_CAP entries; "+N more" consumes the last slot) ----
    (void)renderListWithStrictCapCap(
      d, host.vm_list, host.vm_list_count, VM_CAP,
      y, bulletX, nameStartX, valueR, gapMin, ICON_W, LINE_H
    );

    // ---- LXCs: running/total ----
    d.setCursor(labelX, y); d.print(F("LXCs:"));
    {
      String v = (host.lxcs_running >= 0 && host.lxcs_total >= 0)
                 ? String(host.lxcs_running) + "/" + String(host.lxcs_total)
                 : String("-");
      ui::printRight(d, valueR, y, v);
    }
    y += LINE_H;

    // ---- LXC list (up to LXC_CAP entries; "+N more" consumes the last slot) ----
    (void)renderListWithStrictCapCap(
      d, host.lxc_list, host.lxc_list_count, LXC_CAP,
      y, bulletX, nameStartX, valueR, gapMin, ICON_W, LINE_H
    );

    // no footer

  } while (d.nextPage());
}
