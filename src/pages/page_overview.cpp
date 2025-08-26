#include "page_overview.h"
#include "ui_theme.h"
#include "state.h"
#include "config.h"
#include <WiFi.h> 
#include <Fonts/FreeMono9pt7b.h>

void PageOverview::render(Epd_t &d, const HostState &host, const UiState &ui)
{
  d.firstPage();
  do
  {
    d.fillScreen(GxEPD_WHITE);

    ui::header(d, F("Status"));
    d.setFont(&FreeMono9pt7b);

    const int16_t LINE_H = 22;
    const int16_t labelX = 4;
    const int16_t valueR = d.width() - 4;

    int16_t y = ui::content_top();

    // IP
    d.setCursor(labelX, y);
    d.print(F("IP:"));
    if (host.primary_ipv4[0])
    {
      String ipStr(host.primary_ipv4);
      int slashPos = ipStr.indexOf('/');
      if (slashPos > 0)
      {
        ipStr = ipStr.substring(0, slashPos); // remove CIDR
      }
      ui::printRight(d, valueR, y, ipStr);
    }
    else
    {
      ui::printRight(d, valueR, y, String("-"));
    }
    y += LINE_H;
    // CPU (%)
    d.setCursor(labelX, y);
    d.print(F("CPU:"));
    ui::printRight(d, valueR, y, isnan(host.cpu_percent) ? String("-") : String(host.cpu_percent, 1) + "%");
    y += LINE_H;

    // RAM — "used/total GiB"
    d.setCursor(labelX, y);
    d.print(F("RAM:"));
    if (host.ram_total == 0)
    {
      ui::printRight(d, valueR, y, String("-"));
    }
    else
    {
      float usedGiB = bytesToGiB(host.ram_used);
      float totGiB = bytesToGiB(host.ram_total);
      char buf[32];
      snprintf(buf, sizeof(buf), "%.1f/%.1f GiB", usedGiB, totGiB);
      ui::printRight(d, valueR, y, String(buf));
    }
    y += LINE_H;

    // Case temperature
    d.setCursor(labelX, y);
    d.print(F("Case:"));
    ui::printRight(d, valueR, y, isnan(host.local_temp_c) ? String("-") : String(host.local_temp_c, 1) + "C");
    y += LINE_H;

    // Uptime (days & hours)
    d.setCursor(labelX, y);
    d.print(F("Uptime:"));
    {
      uint32_t up_s = host.uptime_sec;
      uint32_t days = up_s / 86400U;
      uint32_t hours = (up_s % 86400U) / 3600U;
      ui::printRight(d, valueR, y, String(days) + "d " + String(hours) + "h");
    }
    y += LINE_H;

    // Link (moved here, below Uptime)
    d.setCursor(labelX, y);
    d.print(F("Link:"));
    {
      String v;
      if (!ui.lastParseOkMs)
      {
        v = F("No data");
      }
      else
      {
        uint32_t ageS = secsSince(ui.lastParseOkMs);
        bool ok = (ageS <= LINK_TIMEOUT_S);
        char buf[32];
        snprintf(buf, sizeof(buf), "%s (%us)", ok ? "Online" : "Timeout", (unsigned)ageS);
        v = String(buf);
      }
      ui::printRight(d, valueR, y, v);
    }
    // y += LINE_H; // not needed unless you add more lines

    // Footer (kept as before)
    d.setCursor(4, d.height() - 8);
    d.print(F("["));
    d.print(ui.mode == MODE_TOUCH ? F("TOUCH") : F("AUTO"));
    d.print(F("]"));

    d.setCursor(4, d.height() - 8);
d.print(F("["));
d.print(ui.mode == MODE_TOUCH ? F("TOUCH") : F("AUTO"));
d.print(F("]"));

// Draw WiFi RSSI symbol at bottom-right
int16_t rssi = WiFi.RSSI();  // returns dBm
int bars = 0;
if (rssi > -55)      bars = 4; // excellent
else if (rssi > -65) bars = 3; // good
else if (rssi > -75) bars = 2; // fair
else if (rssi > -85) bars = 1; // weak
else                 bars = 0; // none

int16_t bx = d.width() - 28;   // move further left (wider bars)
int16_t by = d.height() - 5;   // baseline

const int barWidth = 4;        // wider bars
const int spacing  = 3;        // horizontal spacing between bars

for (int i = 0; i < 4; i++) {
  int h = (i + 1) * 2 + 4; // bar height + 4px
  int x = bx + i * (barWidth + spacing);
  if (i < bars) {
    d.fillRect(x, by - h, barWidth, h, GxEPD_BLACK);
  } else {
    d.drawRect(x, by - h, barWidth, h, GxEPD_BLACK);
  }
}

  } while (d.nextPage());
}

