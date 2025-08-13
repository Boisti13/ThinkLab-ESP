#include "page_network.h"
#include "ui_theme.h"
#include "state.h"
#include <Fonts/FreeMono9pt7b.h>

// format kbps → "850 kbps" or "1.2 Mbps"
static String fmtRate(float kbps) {
  if (isnan(kbps)) return String("-");
  if (kbps >= 1000.0f) {
    float mbps = kbps / 1000.0f;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f Mbps", mbps);
    return String(buf);
  }
  char buf[16];
  snprintf(buf, sizeof(buf), "%d kbps", (int)(kbps + 0.5f));
  return String(buf);
}

void PageNetwork::render(Epd_t& d, const HostState& host, const UiState& /*ui*/) {
  d.firstPage();
  do {
    d.fillScreen(GxEPD_WHITE);

    ui::header(d, F("Network"));
    d.setFont(&FreeMono9pt7b);

    const int16_t LINE_H = 22;
    const int16_t labelX = 4;
    const int16_t valueR = d.width() - 4;

    int16_t y = ui::content_top();

    // IP
    d.setCursor(labelX, y); d.print(F("IP:"));
    ui::printRight(d, valueR, y, host.primary_ipv4[0] ? String(host.primary_ipv4) : String("-"));
    y += LINE_H;

    // Status
    d.setCursor(labelX, y); d.print(F("Status:"));
    ui::printRight(d, valueR, y, host.ip_status[0] ? String(host.ip_status) : String("-"));
    y += LINE_H;

    // Down / Up (split lines)
    const bool haveRx = !isnan(host.net_rx_kbps);
    const bool haveTx = !isnan(host.net_tx_kbps);
    if (haveRx || haveTx) {
      d.setCursor(labelX, y); d.print(F("Down:"));
      ui::printRight(d, valueR, y, fmtRate(host.net_rx_kbps));
      y += LINE_H;

      d.setCursor(labelX, y); d.print(F("Up:"));
      ui::printRight(d, valueR, y, fmtRate(host.net_tx_kbps));
    }

    // no footer

  } while (d.nextPage());
}
