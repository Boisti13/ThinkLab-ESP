#include "config.h"
#if USE_WIFI

#include "wifi_ota.h"
#include "web_server.h"     // new

#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <esp_wifi.h>

// ===== Macros (override via build_flags / extra.ini) =========================
#ifndef WIFI_SSID
  #define WIFI_SSID "changeme-ssid"
#endif
#ifndef WIFI_PASS
  #define WIFI_PASS "changeme-pass"
#endif
#ifndef HOSTNAME
  #define HOSTNAME "esp32-default"
#endif
#ifndef WEB_USER
  #define WEB_USER "admin"
#endif
#ifndef WEB_PASS
  #define WEB_PASS "admin"
#endif
// ============================================================================

// ---- state -----------------------------------------------------------------
static bool     wifiConnected     = false;
static bool     otaInit           = false;

static uint16_t failStreak        = 0;
static uint32_t lastDisconnectMs  = 0;   // when the link last went down
static uint32_t lastResetMs       = 0;   // last full Wi-Fi restart

// The driver's auto-reconnect handles normal drops. Only if the link stays
// down this long is Wi-Fi restarted from scratch (and then at most this often).
#ifndef WIFI_RESET_AFTER_MS
  #define WIFI_RESET_AFTER_MS 60000UL
#endif

// Upload state (prevents mid-upload Wi-Fi resets & defers reboot)
static volatile bool inUpload      = false;
static bool          pendingReboot = false;

// ---- mDNS + ArduinoOTA ------------------------------------------------------
static void startMDNS() {
  MDNS.end();
  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    MDNS.enableArduino(3232, true);   // lets PlatformIO/Arduino IDE find the board for OTA
  }
}
static void stopMDNS() {
  MDNS.end();
}

static void ensureArduinoOTA() {
  if (otaInit) return;

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(WEB_PASS);
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setMdnsEnabled(false);  // startMDNS() owns mDNS

  ArduinoOTA
    .onStart([]() {
      inUpload = true;           // block watchdog/backoff
      stopMDNS();
      server.stop();             // from web_server.cpp
      // Keep Wi-Fi awake & stable
      WiFi.setSleep(false);
      esp_wifi_set_ps(WIFI_PS_NONE);
    })
    .onEnd([]() {
      pendingReboot = true;
      inUpload = false;
    })
    .onError([](ota_error_t) {
      inUpload = false;
    });

  ArduinoOTA.begin();
  otaInit = true;
}

static void stopArduinoOTA() {
  if (!otaInit) return;
  // end() closes the UDP socket, so the next begin() binds a fresh one
  ArduinoOTA.end();
  otaInit = false;
}

// ---- connect / recovery -----------------------------------------------------
static void wifiBeginFresh() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.setHostname(HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  lastResetMs = millis();
  lastDisconnectMs = lastResetMs;   // counts as "down" until GOT_IP
}

// ---- Wi-Fi events -----------------------------------------------------------
static void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      wifiConnected  = true;
      failStreak     = 0;
      startMDNS();
      ensureArduinoOTA();
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      // Fires again for every failed attempt; auto-reconnect keeps trying.
      if (wifiConnected) lastDisconnectMs = millis();
      wifiConnected = false;
      stopArduinoOTA();
      failStreak++;
      break;

    default: break;
  }
}

// ---- maintenance --------------------------------------------------------------
// No internet check: the board only needs the LAN (web page, OTA), and the
// house has no internet at night by design. Normal drops are handled by the
// driver's auto-reconnect; a full restart only after WIFI_RESET_AFTER_MS down.
static void wifiMaintain() {
  if (inUpload || wifiConnected) return;
  const uint32_t now = millis();
  if (now - lastDisconnectMs >= WIFI_RESET_AFTER_MS && now - lastResetMs >= WIFI_RESET_AFTER_MS) {
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
    delay(100);
    wifiBeginFresh();
  }
}

// ===== Public API ============================================================
void wifiOtaSetup() {
  WiFi.onEvent(onWiFiEvent);
  wifiBeginFresh();     // kick off Wi-Fi (non-blocking)
  webServerSetup();     // new: website lives in its own module
}

void wifiOtaLoop() {
  wifiMaintain();
  server.handleClient();           // web module’s server
  if (wifiConnected && otaInit) {
    ArduinoOTA.handle();
  }
  if (pendingReboot) {
    pendingReboot = false;
    delay(1200);                   // let TCP flush responses
    ESP.restart();
  }
}

// Called by web_server.cpp during HTTP upload
void wifiOta_SetInUpload(bool on) { inUpload = on; }
void wifiOta_RequestReboot()       { pendingReboot = true; }

#endif // USE_WIFI
