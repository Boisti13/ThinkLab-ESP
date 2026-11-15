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
        // Compact age so the line fits: 42s, 25m, 3h
        uint32_t ageS = secsSince(ui.lastParseOkMs);
        bool ok = (ageS <= LINK_TIMEOUT_S);
        char age[12];
        if (ageS < 120)        snprintf(age, sizeof(age), "%us", (unsigned)ageS);
        else if (ageS < 7200)  snprintf(age, sizeof(age), "%um", (unsigned)(ageS / 60));
        else                   snprintf(age, sizeof(age), "%uh", (unsigned)(ageS / 3600));
        char buf[24];
        snprintf(buf, sizeof(buf), "%s %s", ok ? "Online" : "Lost", age);
        v = String(buf);
      }
      ui::printRight(d, valueR, y, v);
    }
    // y += LINE_H; // not needed unless you add more lines

    // Footer: page mode left, Wi-Fi bars right
    d.setCursor(4, d.height() - 8);
    d.print(F("["));
    d.print(ui.mode == MODE_TOUCH ? F("TOUCH") : F("AUTO"));
    d.print(F("]"));

    const int bars = WiFi.isConnected() ? ui::rssi_to_bars(WiFi.RSSI()) : 0;
    ui::wifi_icon(d, d.width() - 17, d.height() - 5, bars);

  } while (d.nextPage());
}

