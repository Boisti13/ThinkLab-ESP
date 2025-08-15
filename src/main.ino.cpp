# 1 "C:\\Users\\KOLB-G~1\\AppData\\Local\\Temp\\tmp32007tp2"
#include <Arduino.h>
# 1 "C:/Users/Kolb-Grunder/Documents/PlatformIO/Projects/hl_hostmon_esp/src/main.ino"





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
#if USE_FANCTRL
#include "modules/fanctrl.h"
#endif
#if USE_EXPERIMENTAL
#include "modules/experimental.h"
#endif


#include "pages/page_overview.h"
#include "pages/page_disks.h"
#include "pages/page_vms.h"
#include "pages/page_network.h"
#include "pages/page_debug.h"

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>


HostState g_host;
UiState g_ui;
DisplayManager g_disp;
SerialClient g_serial;
TouchInput g_touch;

#if USE_WIFI
WifiOta g_wifi;
#endif
#if USE_DALLAS
DallasProbe g_dallas;
#endif


PageOverview g_pageOverview;
PageDisks g_pageDisks;
PageVMs g_pageVMs;
PageNetwork g_pageNetwork;
PageDebug g_pageDebug;

static uint32_t lastDisplayMs = 0;
static bool bootCleared = false;


static inline void renderNow()
{
  g_disp.renderCurrent(g_host, g_ui);
}

static inline void renderDebugDirect()
{
  auto &d = g_disp.display();
  g_pageDebug.render(d, g_host, g_ui);
}
static void splash();
void setup();
void loop();
#line 81 "C:/Users/Kolb-Grunder/Documents/PlatformIO/Projects/hl_hostmon_esp/src/main.ino"
static void splash()
{
  auto &d = g_disp.display();
  const int16_t W = d.width();
  const int16_t H = d.height();
  const char *TITLE = "ThinkLab";

  d.firstPage();
  do
  {
    d.fillScreen(GxEPD_WHITE);
    d.setTextColor(GxEPD_BLACK);


    d.setFont();
    const int len = strlen(TITLE);
    const int16_t padX = 6;
    const int16_t avail = W - 2 * padX;

    int size = avail / (len * 6);
    if (size < 1)
      size = 1;
    if (size > 3)
      size = 3;

    const int16_t titleW = len * 6 * size;
    const int16_t titleH = 8 * size;

    int16_t x = (W - titleW) / 2;
    int16_t y = (H - titleH) / 2 - 6;

    d.setTextSize(size);
    d.setCursor(x, y);
    d.print(TITLE);
    d.setCursor(x + 1, y);
    d.print(TITLE);


    d.setTextSize(1);
    d.setFont(&FreeMono9pt7b);
    String sub = F("Booting...");
    int16_t x1, y1;
    uint16_t w, h;
    d.getTextBounds(sub, 0, 0, &x1, &y1, &w, &h);
    const int16_t subX = (W - (int16_t)w) / 2;
    const int16_t subY = y + titleH + 80;
    d.setCursor(subX, subY);
    d.print(sub);
  } while (d.nextPage());
}


void setup()
{

  Serial.setRxBufferSize(8192);
  Serial.begin(115200);
  Serial.setRxBufferSize(8192);

  g_disp.begin();
  g_touch.begin();

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
#if USE_FAN1
  fan1PwmBegin();
  fan1TachBegin();
#endif

#if USE_FANCTRL
  fanCtrlBegin();
#endif
#if USE_EXPERIMENTAL
  experimentalBegin();
#endif


  g_disp.registerPage(&g_pageOverview, false);
  g_disp.registerPage(&g_pageDisks, false);
  g_disp.registerPage(&g_pageVMs, false);
  g_disp.registerPage(&g_pageNetwork, false);
  g_disp.registerPage(&g_pageDebug, true);

  splash();

  g_serial.begin();

}


void loop()
{

  g_serial.tick(g_host, g_ui);


  if (!bootCleared && g_ui.firstDataReady)
  {
    bootCleared = true;
    lastDisplayMs = millis();
    renderNow();
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
#if USE_FANCTRL
  fanCtrlTick(g_host);
#endif

#if USE_EXPERIMENTAL
  experimentalTick(g_ui);
#endif


  ButtonEvent ev = g_touch.poll();

  if (ev == ButtonEvent::DoubleTap)
  {

    g_ui.tapPending = false;


    g_ui.inDebugMode = !g_ui.inDebugMode;
    g_ui.lastDebugRefresh = 0;


    if (g_ui.inDebugMode)
    {
      renderDebugDirect();
    }
    else
    {
      renderNow();
    }
  }
  else if (ev == ButtonEvent::LongPress)
  {

    g_ui.tapPending = false;


    g_ui.mode = (g_ui.mode == MODE_TOUCH) ? MODE_AUTO : MODE_TOUCH;


    if (g_ui.inDebugMode)
    {
      renderDebugDirect();
    }
    else
    {
      renderNow();
    }
  }
  else if (ev == ButtonEvent::Tap)
  {
    if (!g_ui.inDebugMode)
    {

      g_ui.tapPending = true;
      g_ui.tapDeadlineMs = millis() + TOUCH_DBL_MS;
    }
  }


  if (g_ui.tapPending && (int32_t)(millis() - g_ui.tapDeadlineMs) >= 0)
  {
    g_ui.tapPending = false;


    if (!g_ui.inDebugMode)
    {
      if (millis() > g_ui.advanceArmUntilMs)
      {

        renderNow();
        g_ui.advanceArmUntilMs = millis() + TOUCH_ADVANCE_ARM_MS;
      }
      else
      {

        uint8_t n = g_disp.pageCount(g_ui);
        g_ui.currentPage = (n == 0) ? 0 : (g_ui.currentPage + 1) % n;
        renderNow();
        g_ui.advanceArmUntilMs = 0;
      }
    }
  }


  if (g_ui.inDebugMode)
  {
    uint32_t nowDbg = millis();
    if (nowDbg - g_ui.lastDebugRefresh >= DEBUG_REFRESH_MS)
    {
      renderDebugDirect();
      g_ui.lastDebugRefresh = nowDbg;
    }
  }


  uint32_t now = millis();
  if (!g_ui.inDebugMode && g_ui.mode == MODE_AUTO &&
      (lastDisplayMs != 0) && (now - lastDisplayMs >= DISPLAY_INTERVAL_MS))
  {
    lastDisplayMs = now;
    renderNow();
    uint8_t n = g_disp.pageCount(g_ui);
    g_ui.currentPage = (n == 0) ? 0 : (g_ui.currentPage + 1) % n;
  }
}