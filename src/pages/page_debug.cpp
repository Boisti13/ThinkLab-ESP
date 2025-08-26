#include "page_debug.h"
#include "ui_theme.h"
#include "state.h"
#include "config.h"
#include <Fonts/FreeMono9pt7b.h>

#if USE_FAN1
#include "modules/fan1_pwm.h"
#include "modules/fan1_tach.h"
#endif
#if USE_FAN2_TACH
#include "modules/fan2_tach.h"
#endif

#if USE_DALLAS
  #include "modules/dallas.h"   // provides dallasGetTempC() or similar
#endif

#if USE_WIFI
  #include <WiFi.h>
#endif

void PageDebug::render(Epd_t &d, const HostState &host, const UiState &ui)
{
  d.firstPage();
  do
  {
    d.fillScreen(GxEPD_WHITE);

    ui::header(d, F("Debug"));
    d.setFont(&FreeMono9pt7b);

    // layout knobs (LOCAL)
    const int16_t LINE_H = 15;
    const int16_t labelX = 4;
    const int16_t valueR = d.width() - 4;
    int16_t y = ui::content_top();

    // FW version
    d.setCursor(labelX, y);
    d.print(F("FW:"));
    ui::printRight(d, valueR, y, String(FW_VERSION));
    y += LINE_H;

    // RX age
    d.setCursor(labelX, y);
    d.print(F("RX age:"));
    ui::printRight(d, valueR, y, ui.lastParseOkMs ? String(secsSince(ui.lastParseOkMs)) + "s" : String("-"));
    y += LINE_H;

    // OK/ERR
    d.setCursor(labelX, y);
    d.print(F("OK/ERR:"));
    ui::printRight(d, valueR, y, String(ui.parseOkCount) + "/" + String(ui.parseErrCount));
    y += LINE_H;

    // JSON len
    d.setCursor(labelX, y);
    d.print(F("JSON len:"));
    ui::printRight(d, valueR, y, String(ui.lastJsonLen) + " B");
    y += LINE_H;

    // Poll
    d.setCursor(labelX, y);
    d.print(F("Poll:"));
    ui::printRight(d, valueR, y, String(POLL_INTERVAL_MS / 1000.0f, 1) + "s");
    y += LINE_H;

    // Mode
    d.setCursor(labelX, y);
    d.print(F("Mode:"));
    ui::printRight(d, valueR, y, ui.mode == MODE_TOUCH ? String("TOUCH") : String("AUTO"));
    y += LINE_H;

    // Debug enabled
    // d.setCursor(labelX, y); d.print(F("Debug:"));
    // ui::printRight(d, valueR, y, ui.debugEnabled ? String("ON") : String("OFF"));
    // y += LINE_H;

    // --- ADD: Fan PWM / RPM (right-aligned) ---
#if USE_FAN1 && DBG_SHOW_FAN1_PWM
  d.setCursor(labelX, y);
  d.print(F("Fan1 PWM:"));
  ui::printRight(d, valueR, y, String(fan1PwmGetPercent()) + "%");
  y += LINE_H;
#endif

#if USE_FAN1 && DBG_SHOW_FAN1_RPM
  d.setCursor(labelX, y);
  d.print(F("Fan1 RPM:"));
  {
    int rpm1 = fan1TachGetRPM();
    ui::printRight(d, valueR, y, rpm1 < 0 ? String("—") : String(rpm1));
  }
  y += LINE_H;
#endif

#if USE_FAN2_TACH && DBG_SHOW_FAN2_RPM
  d.setCursor(labelX, y);
  d.print(F("Fan2 RPM:"));
  {
    int rpm2 = fan2TachGetRPM();
    ui::printRight(d, valueR, y, rpm2 < 0 ? String("—") : String(rpm2));
  }
  y += LINE_H;
#endif

#if DBG_SHOW_FAN_BLOCK && DBG_SHOW_FAN_CMD
  d.setCursor(labelX, y);
  d.print(F("FanCmd"));
  ui::printRight(d, valueR, y, isnan(host.fan_duty_cmd) ? String("-") : String(host.fan_duty_cmd, 1) + "%");
  y += LINE_H;
#endif

#if DBG_SHOW_FAN_BLOCK && DBG_SHOW_FAN_OUT
  d.setCursor(labelX, y);
  d.print(F("FanOut"));
  ui::printRight(d, valueR, y, isnan(host.fan_duty_filt) ? String("-") : String(host.fan_duty_filt, 1) + "%");
  y += LINE_H;
#endif

#if DBG_SHOW_FAN_BLOCK && DBG_SHOW_FAN_ACT
  d.setCursor(labelX, y);
  d.print(F("FanAct"));
  ui::printRight(d, valueR, y, host.fan_active ? String("ON") : String("OFF"));
  y += LINE_H;
#endif

// Wi‑Fi status + IP (single line): shows local IP if connected, otherwise "OFF"
#if USE_WIFI && DBG_SHOW_WIFI
  d.setCursor(labelX, y);
  d.print(F("W:"));
  String wifiStr;
  if (WiFi.isConnected()) {                 // ESP32 Arduino core helper
    IPAddress ip = WiFi.localIP();
    wifiStr = ip.toString();                // e.g., "192.168.1.42"
  } else {
    wifiStr = F("OFF");
  }
  ui::printRight(d, valueR, y, wifiStr);
  y += LINE_H;
#endif

// Wi-Fi RSSI (signal strength in dBm)
#if USE_WIFI && DBG_SHOW_WIFI_RSSI
  d.setCursor(labelX, y);
  d.print(F("RSSI:"));
  String rssiStr;
  if (WiFi.isConnected()) {
    rssiStr = String(WiFi.RSSI()) + " dBm";
  } else {
    rssiStr = F("—");
  }
  ui::printRight(d, valueR, y, rssiStr);
  y += LINE_H;
#endif

// Dallas / Case temperature (selectable)
#if USE_DALLAS && DBG_SHOW_DALLAS
  d.setCursor(labelX, y);
  d.print(F("Case:"));
  float tC = host.local_temp_c;  
  if (isnan(tC)) {
    ui::printRight(d, valueR, y, String("—"));
  } else {
    ui::printRight(d, valueR, y, String(tC, 1) + "°C");
  }
  y += LINE_H;
#endif

  } while (d.nextPage());
}
