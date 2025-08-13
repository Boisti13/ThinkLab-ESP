#include "config.h"
#if USE_WIFI
#include "wifi_ota.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

static const char* WIFI_SSID = "YOUR_SSID";
static const char* WIFI_PASS = "YOUR_PASSWORD";
static const char* OTA_HOST  = "hl-hostmon-esp";
static const char* OTA_PASS  = ""; // optional

void WifiOta::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) delay(100);
  MDNS.begin(OTA_HOST);
  ArduinoOTA.setHostname(OTA_HOST);
  if (strlen(OTA_PASS)) ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.begin();
}
void WifiOta::tick() { ArduinoOTA.handle(); }
#endif
