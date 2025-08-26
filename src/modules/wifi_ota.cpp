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

static uint32_t nextAttemptMs     = 0;
static uint8_t  retryExp          = 0;   // backoff 0..4 (3s..48s)
static uint8_t  quickKickTries    = 0;
static uint16_t failStreak        = 0;
static uint32_t lastDisconnectMs  = 0;

// Internet watchdog (DNS-based)
static uint32_t lastInetCheckMs   = 0;

// Upload state (prevents mid-upload Wi-Fi resets & defers reboot)
static volatile bool inUpload      = false;
static bool          pendingReboot = false;

// ---- small helpers ----------------------------------------------------------
static bool checkInternetSimple() {
  IPAddress addr;
  return WiFi.hostByName("google.com", addr);
}

// ---- mDNS + ArduinoOTA ------------------------------------------------------
static void startMDNS() {
  MDNS.end();
  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
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
  // no explicit end(); otaInit gates handle() calls
  otaInit = false;
}

// ---- backoff & recovery helpers --------------------------------------------
static void scheduleNextAttempt(uint32_t baseMs = 3000UL) {
  uint32_t delayMs = baseMs << (retryExp <= 4 ? retryExp : 4); // 3s..48s
  if (delayMs > 60000UL) delayMs = 60000UL;
  nextAttemptMs = millis() + delayMs;
  if (retryExp < 10) retryExp++;
}

static void wifiBeginFresh() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.setHostname(HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  retryExp = 0;
  quickKickTries = 0;
  scheduleNextAttempt(1000); // first check soon
}

// ---- Wi-Fi events -----------------------------------------------------------
static void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      wifiConnected  = true;
      retryExp       = 0;
      quickKickTries = 0;
      failStreak     = 0;
      startMDNS();
      ensureArduinoOTA();
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      wifiConnected = false;
      stopArduinoOTA();
      lastDisconnectMs = millis();
      failStreak++;
      if (quickKickTries < 3) {
        esp_wifi_connect();
        quickKickTries++;
      }
      scheduleNextAttempt();
      break;

    default: break;
  }
}

// ---- maintenance (watchdog + DNS connectivity) -----------------------------
static void wifiMaintain() {
  if (inUpload) return; // never tamper during uploads

  // If link is up, periodically verify real connectivity via DNS (30s)
  if (wifiConnected && (millis() - lastInetCheckMs) > 30000UL) {
    lastInetCheckMs = millis();
    if (!checkInternetSimple()) {
      // DNS unreachable → full Wi-Fi shutdown + delayed restart
      WiFi.disconnect(true, true);
      WiFi.mode(WIFI_OFF);
      wifiConnected = false;
      stopArduinoOTA();
      lastDisconnectMs = millis();
      nextAttemptMs = millis() + 30000UL;
      retryExp = 0;
      return;
    }
  }

  if (wifiConnected) return;

  // Time for a scheduled reconnect?
  if ((int32_t)(millis() - nextAttemptMs) >= 0) {
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.setHostname(HOSTNAME);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    scheduleNextAttempt();
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
