#include "page_network.h"
#include "ui_theme.h"
#include "state.h"
#include <Fonts/FreeMono9pt7b.h>

#if USE_WIFI
#include <WiFi.h>
#endif

// format kbps → "850 kbps" or "1.2 Mbps"
static String fmtRate(float kbps)
{
  if (isnan(kbps))
    return String("-");
  if (kbps >= 1000.0f)
  {
    float mbps = kbps / 1000.0f;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f Mbps", mbps);
    return String(buf);
  }
  char buf[16];
  snprintf(buf, sizeof(buf), "%d kbps", (int)(kbps + 0.5f));
  return String(buf);
}

void PageNetwork::render(Epd_t &d, const HostState &host, const UiState & /*ui*/)
{
  d.firstPage();
  do
  {
    d.fillScreen(GxEPD_WHITE);

    ui::header(d, F("Network"));
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

    // Status
    d.setCursor(labelX, y);
    d.print(F("Status:"));
    ui::printRight(d, valueR, y, host.ip_status[0] ? String(host.ip_status) : String("-"));
    y += LINE_H;

    // Down / Up (split lines)
    const bool haveRx = !isnan(host.net_rx_kbps);
    const bool haveTx = !isnan(host.net_tx_kbps);
    if (haveRx || haveTx)
    {
      d.setCursor(labelX, y);
      d.print(F("Down:"));
      ui::printRight(d, valueR, y, fmtRate(host.net_rx_kbps));
      y += LINE_H;

      d.setCursor(labelX, y);
      d.print(F("Up:"));
      ui::printRight(d, valueR, y, fmtRate(host.net_tx_kbps));
    }

    // Wi‑Fi status + IP (single line): shows local IP if connected, otherwise "OFF"
#if USE_WIFI
    // Draw Wi-Fi icon instead of "W:"
    int bars = 0;
    if (WiFi.isConnected()) {
      bars = ui::rssi_to_bars(WiFi.RSSI());
    }
    // icon center: ~8 px right of labelX, baseline aligned with text
    ui::wifi_icon(d, labelX + 15, y, bars);

    // Right-aligned value: IP address or OFF
    String wifiStr;
    if (WiFi.isConnected()) {
      IPAddress ip = WiFi.localIP();
      wifiStr = ip.toString(); // e.g., "192.168.1.42"
    } else {
      wifiStr = F("OFF");
    }
    ui::printRight(d, valueR, y, wifiStr);
    y += LINE_H;
#endif
    // no footer

  } while (d.nextPage());
}
