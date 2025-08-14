/*
  hl-hostmon-esp — main (v0.1.x)
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

#if USE_FAN1
  #include "modules/fan1_pwm.h"
  #include "modules/fan1_tach.h"
#endif
#if USE_FAN2_TACH
  #include "modules/fan2_tach.h"
#endif
#if USE_EXPERIMENTAL
  #include "modules/experimental.h"
#endif

// Pages
#include "pages/page_overview.h"
#include "pages/page_disks.h"
#include "pages/page_vms.h"
#include "pages/page_network.h"
#include "pages/page_debug.h"

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// ==== Globals ====
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

// Page objects
PageOverview   g_pageOverview;
PageDisks      g_pageDisks;
PageVMs        g_pageVMs;
PageNetwork    g_pageNetwork;
PageDebug      g_pageDebug;

static uint32_t lastDisplayMs = 0;   // rotation timer (0 means not started)
static bool     bootCleared   = false; // leave splash once first data arrives

// ---- helpers ----
static inline void renderNow() {
  g_disp.renderCurrent(g_host, g_ui);
}

static inline void renderDebugDirect() {
  auto &d = g_disp.display();
  g_pageDebug.render(d, g_host, g_ui);
}

// ---- splash ----
static void splash() {
  auto& d = g_disp.display();
  const int16_t W = d.width();
  const int16_t H = d.height();
  const char* TITLE = "ThinkLab";

  d.firstPage();
  do {
    d.fillScreen(GxEPD_WHITE);
    d.setTextColor(GxEPD_BLACK);

    // Headline (classic font, clamped size, faux-bold)
    d.setFont(); // classic 5x7
    const int len = strlen(TITLE);
    const int16_t padX = 6;
    const int16_t avail = W - 2 * padX;

    int size = avail / (len * 6);     // ~6 px/char at size=1
    if (size < 1) size = 1;
    if (size > 3) size = 3;           // clamp so it’s not gigantic

    const int16_t titleW = len * 6 * size;
    const int16_t titleH = 8 * size;

    int16_t x = (W - titleW) / 2;
    int16_t y = (H - titleH) / 2 - 6;

    d.setTextSize(size);
    d.setCursor(x, y);     d.print(TITLE);
    d.setCursor(x + 1, y); d.print(TITLE); // faux bold

    // Subtitle
    d.setTextSize(1);
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

// ======================= setup =======================
void setup() {
  // USB CDC (no debug prints — Serial is for host protocol)
  Serial.setRxBufferSize(8192);
  Serial.begin(115200);
  Serial.setRxBufferSize(8192);

  g_disp.begin();      // init EPD (rotation inside DM)
  g_touch.begin();     // button on TOUCH_PIN (safe if not connected)

#if USE_WIFI
  g_wifi.begin();
#endif
#if USE_DALLAS
  g_dallas.begin();
#endif
#if USE_FAN1
  fan1PwmBegin();
  fan1TachBegin();
#endif
#if USE_FAN2_TACH
  fan2TachBegin();
#endif
#if USE_EXPERIMENTAL
  experimentalBegin();
#endif

  // Register pages; keep Debug registered as debug-only (not in normal roll)
  g_disp.registerPage(&g_pageOverview, /*isDebug*/false);
  g_disp.registerPage(&g_pageDisks,    /*isDebug*/false);
  g_disp.registerPage(&g_pageVMs,      /*isDebug*/false);
  g_disp.registerPage(&g_pageNetwork,  /*isDebug*/false);
  g_disp.registerPage(&g_pageDebug,    /*isDebug*/true); // not in normal rotation

  splash();

  g_serial.begin(); // sends INFO once
  // NOTE: do NOT set lastDisplayMs here; we start it when first data arrives
}

// ======================= loop =======================
void loop() {
  // --- Host serial client (read, parse, poll GET interval)
  g_serial.tick(g_host, g_ui);

  // Leave splash automatically once first valid data arrives (Option B)
  if (!bootCleared && g_ui.firstDataReady) {
    bootCleared   = true;
    lastDisplayMs = millis();   // start auto-rotation timer now
    renderNow();                // show first real page
  }

#if USE_WIFI
  g_wifi.tick();
#endif
#if USE_DALLAS
  g_dallas.tick();
  g_host.local_temp_c = g_dallas.lastC();
#endif
#if USE_FAN1
  fan1TachTick();
#endif
#if USE_FAN2_TACH
  fan2TachTick();
#endif
#if USE_EXPERIMENTAL
  experimentalTick(g_ui);
#endif

  // --- Handle touch events (non-blocking to allow double-tap)
  ButtonEvent ev = g_touch.poll();

  if (ev == ButtonEvent::DoubleTap) {
    // Cancel any pending single-tap
    g_ui.tapPending = false;

    // Toggle dedicated Debug mode (not part of normal rotation)
    g_ui.inDebugMode      = !g_ui.inDebugMode;
    g_ui.lastDebugRefresh = 0; // force immediate refresh

    // Render immediately: Debug page directly, or normal page via DM
    if (g_ui.inDebugMode) {
      renderDebugDirect();
    } else {
      renderNow();
    }

  } else if (ev == ButtonEvent::LongPress) {
    // Cancel any pending tap
    g_ui.tapPending = false;

    // Toggle TOUCH/AUTO mode
    g_ui.mode = (g_ui.mode == MODE_TOUCH) ? MODE_AUTO : MODE_TOUCH;

    // Re-render current view (Debug or normal)
    if (g_ui.inDebugMode) {
      renderDebugDirect();
    } else {
      renderNow();
    }

  } else if (ev == ButtonEvent::Tap) {
    if (!g_ui.inDebugMode) {
      // Defer single-tap action until double-tap window expires (prevents e-ink blocking)
      g_ui.tapPending    = true;
      g_ui.tapDeadlineMs = millis() + TOUCH_DBL_MS; // e.g., 350 ms
    }
  }

  // --- Commit pending single-tap after double-tap window passes
  if (g_ui.tapPending && (int32_t)(millis() - g_ui.tapDeadlineMs) >= 0) {
    g_ui.tapPending = false;

    // In normal mode: first tap = refresh, second within ADVANCE window = advance
    if (!g_ui.inDebugMode) {
      if (millis() > g_ui.advanceArmUntilMs) {
        // First tap: refresh current page & arm the advance window
        renderNow();
        g_ui.advanceArmUntilMs = millis() + TOUCH_ADVANCE_ARM_MS; // e.g., 20s
      } else {
        // Second tap inside the advance window: advance page & disarm
        uint8_t n = g_disp.pageCount(g_ui);
        g_ui.currentPage = (n == 0) ? 0 : (g_ui.currentPage + 1) % n;
        renderNow();
        g_ui.advanceArmUntilMs = 0;
      }
    }
  }

  // --- Periodic refresh in dedicated Debug mode
  if (g_ui.inDebugMode) {
    uint32_t nowDbg = millis();
    if (nowDbg - g_ui.lastDebugRefresh >= DEBUG_REFRESH_MS) {
      renderDebugDirect();
      g_ui.lastDebugRefresh = nowDbg;
    }
  }

  // --- Auto mode page rotation (paused in debug mode)
  uint32_t now = millis();
  if (!g_ui.inDebugMode && g_ui.mode == MODE_AUTO &&
      (lastDisplayMs != 0) && (now - lastDisplayMs >= DISPLAY_INTERVAL_MS)) {
    lastDisplayMs = now;
    renderNow();
    uint8_t n = g_disp.pageCount(g_ui);
    g_ui.currentPage = (n == 0) ? 0 : (g_ui.currentPage + 1) % n;
  }
}
