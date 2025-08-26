#include "config.h"
#if USE_WIFI

#include "wifi_ota.h"

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include <esp_wifi.h>   // esp_wifi_* for radio control

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
#ifndef BUILD_VERSION
  #define BUILD_VERSION "dev"
#endif
// ============================================================================

// ---- globals ----
WebServer server(80);

static bool     wifiConnected     = false;
static bool     otaInit           = false;

static uint32_t nextAttemptMs     = 0;
static uint8_t  retryExp          = 0;   // backoff 0..4 (3s..48s)
static uint8_t  quickKickTries    = 0;   // immediate esp_wifi_connect() nudges
static uint16_t failStreak        = 0;   // consecutive disconnects
static uint32_t lastDisconnectMs  = 0;

// Internet watchdog (DNS-based)
static uint32_t lastInetCheckMs   = 0;

// Upload state (prevents mid-upload Wi-Fi resets & defers reboot)
static volatile bool inUpload      = false;
static bool          pendingReboot = false;

// ---- helpers ----
static bool checkInternetSimple() {
  IPAddress addr;
  return WiFi.hostByName("google.com", addr);
}

static int rssiToBars(int rssi) {
  if (rssi >= -55) return 4;
  if (rssi >= -65) return 3;
  if (rssi >= -75) return 2;
  if (rssi >= -85) return 1;
  return 0;
}
static const char* rssiLabel(int rssi) {
  if (rssi >= -55) return "excellent";
  if (rssi >= -65) return "good";
  if (rssi >= -75) return "fair";
  if (rssi >= -85) return "weak";
  return "bad";
}
static bool checkAuth() {
  if (!server.authenticate(WEB_USER, WEB_PASS)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

// ---- HTML pages ----
static void handleRoot() {
  if (!checkAuth()) return;

  const bool linkUp = WiFi.isConnected();
  const IPAddress ip = WiFi.localIP();
  const long rssi   = linkUp ? WiFi.RSSI() : -127;
  const int bars    = rssiToBars((int)rssi);

  unsigned long secs = millis() / 1000;
  unsigned long days = secs / 86400; secs %= 86400;
  unsigned long hours = secs / 3600; secs %= 3600;
  unsigned long mins  = secs / 60;

  String html;
  html.reserve(2600);
  html += "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
          "<style>body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;"
          "max-width:720px;margin:24px auto;padding:0 12px}"
          "h2{margin:0 0 12px}"
          ".card{border:1px solid #ddd;border-radius:12px;padding:16px;margin:12px 0}"
          ".row{display:flex;justify-content:space-between;margin:6px 0}"
          ".tag{font:12px/1.6 monospace;background:#f4f4f4;padding:2px 6px;border-radius:6px}"
          ".bars{display:inline-block;vertical-align:middle;margin-left:8px}"
          ".bar{display:inline-block;width:6px;margin-right:2px;background:#bbb;border-radius:2px}"
          ".bar.on{background:#222}</style>";

  html += "<h2>ThinkLab Web Viewer</h2>";

  html += "<div class=card><b>ESP32-C3</b>";
  html += "<div class=row><div>Hostname</div><div class=tag>" + String(HOSTNAME) + "</div></div>";
  html += "<div class=row><div>IP</div><div class=tag>" + (linkUp ? ip.toString() : "-") + "</div></div>";
  html += "<div class=row><div>RSSI</div><div class=tag>";
  if (linkUp) {
    html += String(rssi) + " dBm (" + rssiLabel((int)rssi) + ")";
    html += "<span class=bars>";
    for (int i = 0; i < 4; i++) {
      html += String("<span class='bar ") + (i < bars ? "on" : "") +
              "' style='height:" + String((i+1)*6) + "px'></span>";
    }
    html += "</span>";
  } else {
    html += "disconnected";
  }
  html += "</div></div>";

  html += "<div class=row><div>Uptime</div><div class=tag>" +
          String(days) + "d " + String(hours) + "h " + String(mins) + "m</div></div>";

  html += "<div class=row><div>SSID</div><div class=tag>" +
          (linkUp ? WiFi.SSID() : "-") + "</div></div>";

  html += "<div class=row><div>Build</div><div class=tag>" + String(BUILD_VERSION) + "</div></div>";
  html += "</div>";

  html += "<div class=card><b>Proxmox</b>";
  html += "<div class=row><div>IP:Port</div><div class=tag>-</div></div>";
  html += "<div class=row><div>Uptime</div><div class=tag>-</div></div>";
  html += "</div>";

  html += "<div class=card><b>Actions</b>";
  html += "<div class=row><div>Firmware</div><div><a href='/update'>Upload</a></div></div>";
  html += "<div class=row><div>API</div><div><a href='/status.json'>status.json</a></div></div>";
  html += "</div>";

  html += "<script>setTimeout(()=>location.reload(),5000)</script>";

  server.send(200, "text/html", html);
}

static void handleStatusJson() {
  if (!checkAuth()) return;
  const bool linkUp = WiFi.isConnected();
  String out;
  out.reserve(256);
  out += "{";
  out += "\"hostname\":\"" + String(HOSTNAME) + "\",";
  out += "\"ip\":\"" + (linkUp ? WiFi.localIP().toString() : String("")) + "\",";
  out += "\"ssid\":\"" + String(linkUp ? WiFi.SSID() : "") + "\",";
  out += "\"rssi_dbm\":" + String(linkUp ? WiFi.RSSI() : -127) + ",";
  out += "\"build\":\"" + String(BUILD_VERSION) + "\"";
  out += "}";
  server.send(200, "application/json", out);
}

static void handleUpdatePage() {
  if (!checkAuth()) return;
  String html;
  html.reserve(800);
  html += "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
          "<h2>Upload .bin</h2>"
          "<form method='POST' action='/update' enctype='multipart/form-data'>"
          "<input type='file' name='firmware' accept='.bin' required>"
          "<button type='submit'>Update</button>"
          "</form>";
  server.send(200, "text/html", html);
}

// ---- Web-OTA upload handler (guard + deferred reboot) ----------------------
static void handleUpdateUpload() {
  if (!checkAuth()) return;
  HTTPUpload& upload = server.upload();

  switch (upload.status) {
    case UPLOAD_FILE_START: {
      inUpload = true; // pause watchdog/reconnect logic
      size_t maxSpace = ESP.getFreeSketchSpace();
      if (!Update.begin(maxSpace)) {   // alternatively: Update.begin(UPDATE_SIZE_UNKNOWN);
        server.send(500, "text/plain", "Update begin failed");
        inUpload = false;
        return;
      }
      break;
    }
    case UPLOAD_FILE_WRITE: {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.end();
        server.send(500, "text/plain", "Flash write failed");
        inUpload = false;
        return;
      }
      break;
    }
    case UPLOAD_FILE_END: {
      if (Update.end(true) && !Update.hasError()) {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html",
          "<h3>Update successful.</h3><p>Rebooting… you can close this tab.</p>");
        pendingReboot = true;  // reboot later from loop
      } else {
        server.send(500, "text/plain", "Update failed");
      }
      inUpload = false;
      break;
    }
    case UPLOAD_FILE_ABORTED: {
      Update.end();
      server.send(500, "text/plain", "Update aborted");
      inUpload = false;
      break;
    }
    default: break;
  }
}

// ---- services: mDNS + ArduinoOTA ----
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
      // Any ArduinoOTA (espota) upload just began.
      inUpload = true;            // block watchdog/backoff
      stopMDNS();                 // free sockets/memory
      server.stop();              // avoid web handler interference
      // Keep WiFi awake & stable
      WiFi.setSleep(false);
      esp_wifi_set_ps(WIFI_PS_NONE);
    })
    .onEnd([]() {
      // Mark for reboot; let loop() flush to uploader first
      pendingReboot = true;
      inUpload = false;
    })
    .onError([](ota_error_t) {
      // Resume normal ops on failure
      inUpload = false;
    });

  ArduinoOTA.begin();
  otaInit = true;
}

static void stopArduinoOTA() {
  if (!otaInit) return;
  // No explicit end(); the otaInit flag just gates handle() calls.
  otaInit = false;
}

// ---- backoff & recovery helpers ----
static void scheduleNextAttempt(uint32_t baseMs = 3000UL) {
  uint32_t delayMs = baseMs << (retryExp <= 4 ? retryExp : 4); // 3s,6s,12s,24s,48s
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

// Event-driven updates
static void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      wifiConnected     = true;
      retryExp          = 0;
      quickKickTries    = 0;
      failStreak        = 0;
      startMDNS();
      ensureArduinoOTA();
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      wifiConnected     = false;
      stopArduinoOTA();
      lastDisconnectMs  = millis();
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

// Timed maintenance / escalation ladder + internet check
static void wifiMaintain() {
  // Avoid *any* connectivity tampering during OTA upload
  if (inUpload) return;

  // If link is up, periodically verify real connectivity via DNS
  if (wifiConnected && (millis() - lastInetCheckMs) > 30000UL) { // every 30s
    lastInetCheckMs = millis();
    if (!checkInternetSimple()) {
      // DNS unreachable → full Wi-Fi shutdown + delayed restart
      WiFi.disconnect(true, true);   // drop connection & clear config
      WiFi.mode(WIFI_OFF);           // fully stop Wi-Fi radio
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
    // Bring Wi-Fi back online
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.setHostname(HOSTNAME);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    scheduleNextAttempt();
  }
}

// ---- Web server wiring ----
static void setupWeb() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status.json", HTTP_GET, handleStatusJson);
  server.on("/update", HTTP_GET, handleUpdatePage);
  server.on("/update", HTTP_POST, [](){}, handleUpdateUpload);
  server.onNotFound([]() {
    if (!checkAuth()) return;
    server.send(404, "text/plain", "Not found");
  });
  server.begin();
}

// ===== Public API =====
void wifiOtaSetup() {
  WiFi.onEvent(onWiFiEvent);
  wifiBeginFresh();  // kick off Wi-Fi (non-blocking)
  setupWeb();        // web can start before link is up
}

void wifiOtaLoop() {
  wifiMaintain();
  server.handleClient();
  if (wifiConnected && otaInit) {
    ArduinoOTA.handle();
  }
  if (pendingReboot) {
    pendingReboot = false;
    // Give TCP a moment to flush the response back to the uploader
    delay(1200);
    ESP.restart();
  }
}
#endif
