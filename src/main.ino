/*
  hl-hostmon-esp — modular skeleton (v0.1.0 baseline)
  Board: ESP32-C3 (Super Mini)
*/

#include <Arduino.h>
#include "config.h"
#include "state.h"
#include "display_manager.h"
#include "serial_client.h"
#include "touch.h"

#if USE_WIFI
  #include "modules/wifi_ota.h"
#endif
#if USE_DALLAS
  #include "modules/dallas.h"
#endif
#if USE_FANCTRL
  #include "modules/fan.h"
#endif


// Pages
#include "pages/page_overview.h"
#include "pages/page_disks.h"
#include "pages/page_vms.h"
#include "pages/page_network.h"
#include "pages/page_debug.h"

HostState      g_host;
UiState        g_ui;
DisplayManager g_disp;
SerialClient   g_serial;
TouchInput     g_touch;

#if USE_WIFI
WifiOta        g_wifi;
#endif
#if USE_DALLAS
DallasProbe    g_dallas;
#endif
#if USE_FANCTRL
FanController  g_fan;
#endif

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// Pages (owned statically)
PageOverview   g_pageOverview;
PageDisks      g_pageDisks;
PageVMs        g_pageVMs;
PageNetwork    g_pageNetwork;
PageDebug      g_pageDebug;

static uint32_t lastDisplayMs = 0;

static void splash() {
  auto& d = g_disp.display();
  const int16_t W = d.width();
  const int16_t H = d.height();
  const char* TITLE = "ThinkLab";

  d.firstPage();
  do {
    d.fillScreen(GxEPD_WHITE);
    d.setTextColor(GxEPD_BLACK);

    // --- Headline (classic font, clamped size, faux-bold) ---
    d.setFont(); // classic 5x7 font (scalable)
    const int len = strlen(TITLE);
    const int16_t padX = 6;
    const int16_t avail = W - 2 * padX;

    int size = avail / (len * 6);        // ~6 px/char at size=1
    if (size < 1) size = 1;
    if (size > 3) size = 3;              // <-- clamp so it’s not gigantic (tweak 2–3 to taste)

    const int16_t titleW = len * 6 * size;
    const int16_t titleH = 8 * size;

    int16_t x = (W - titleW) / 2;
    int16_t y = (H - titleH) / 2 - 6;

    d.setTextSize(size);
    d.setCursor(x, y);        d.print(TITLE);
    d.setCursor(x + 1, y);    d.print(TITLE);   // faux bold

    // --- Subtitle ---
    d.setTextSize(1);                   // reset classic scaling
    d.setFont(&FreeMono9pt7b);
    String sub = F("Booting...");
    int16_t x1, y1; uint16_t w, h;
    d.getTextBounds(sub, 0, 0, &x1, &y1, &w, &h);
    const int16_t subX = (W - (int16_t)w) / 2;
    const int16_t subY = y + titleH + 80;
    d.setCursor(subX, subY);
    d.print(sub);
  } while (d.nextPage());
}


void setup() {
  // USB CDC (no debug prints to keep host link clean)
  Serial.setRxBufferSize(8192);
  Serial.begin(115200);
  Serial.setRxBufferSize(8192);

  g_disp.begin();            // silent init; rotation set inside
  g_touch.begin();           // button on PIN_TOUCH (safe if not connected)

#if USE_WIFI
  g_wifi.begin();
#endif
#if USE_DALLAS
  g_dallas.begin();
#endif
#if USE_FANCTRL
  g_fan.begin();
#endif

  // Register pages; mark Debug as debug-only
  g_disp.registerPage(&g_pageOverview, /*isDebug*/false);
  g_disp.registerPage(&g_pageDisks,    /*isDebug*/false);
  g_disp.registerPage(&g_pageVMs,      /*isDebug*/false);
  g_disp.registerPage(&g_pageNetwork,  /*isDebug*/false);
  g_disp.registerPage(&g_pageDebug,    /*isDebug*/true);  // in roll only when g_ui.debugEnabled==true

  splash();

  g_serial.begin(); // sends INFO once
  lastDisplayMs = millis();
}

void loop() {
  // Serial client: reads / parses / polls GET at interval
  g_serial.tick(g_host, g_ui);

  // Optional modules
#if USE_WIFI
  g_wifi.tick();
#endif
#if USE_DALLAS
  g_dallas.tick();
  g_host.local_temp_c = g_dallas.lastC(); 
#endif
#if USE_FANCTRL
  g_fan.tick();
#endif

  // Touch events (if button attached)
  switch (g_touch.poll()) {
    case BTN_VLONG:
      g_ui.debugEnabled = !g_ui.debugEnabled;
      g_disp.toast(g_ui.debugEnabled ? F("Debug: ON") : F("Debug: OFF"));
      break;
    case BTN_LONG:
      g_ui.mode = (g_ui.mode == MODE_TOUCH) ? MODE_AUTO : MODE_TOUCH;
      g_disp.toast(g_ui.mode == MODE_TOUCH ? F("Mode: TOUCH") : F("Mode: AUTO"));
      break;
    case BTN_SHORT:
      if (g_ui.mode == MODE_TOUCH) {
        // One-pass cycle through the enabled pages
        uint8_t pages = g_disp.pageCount(g_ui);
        for (uint8_t p = 0; p < pages; ++p) {
          g_ui.currentPage = p;
          g_disp.renderCurrent(g_host, g_ui);
          delay(PAGE_DWELL_MS);
        }
      } else {
        g_disp.renderCurrent(g_host, g_ui);
      }
      break;
    case BTN_NONE:
    default:
      break;
  }

  // Auto mode page rotation
  uint32_t now = millis();
  if (g_ui.mode == MODE_AUTO && (now - lastDisplayMs >= DISPLAY_INTERVAL_MS)) {
    lastDisplayMs = now;
    g_disp.renderCurrent(g_host, g_ui);
    // advance to next enabled page
    uint8_t pages = g_disp.pageCount(g_ui);
    g_ui.currentPage = (pages == 0) ? 0 : (g_ui.currentPage + 1) % pages;
  }

  // First data render safeguard (if you want it): render once after first parse
  if (g_ui.firstDataReady && !g_ui.firstRenderDone) {
    g_ui.firstRenderDone = true;
    g_disp.renderCurrent(g_host, g_ui);
  }
}
