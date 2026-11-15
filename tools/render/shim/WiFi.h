// Stand-in for the ESP32 WiFi object (tools/render): values come from render.cpp.
#pragma once
#include "Arduino.h"
#define WL_CONNECTED 3
struct IPAddress {
  uint8_t o[4] = {0, 0, 0, 0};
  String toString() const {
    char b[16];
    snprintf(b, sizeof b, "%u.%u.%u.%u", o[0], o[1], o[2], o[3]);
    return String(b);
  }
};
struct FakeWiFi {
  bool connected = true;
  int rssi = -52;
  IPAddress ip;
  int status() const { return connected ? WL_CONNECTED : 0; }
  bool isConnected() const { return connected; }
  int RSSI() const { return rssi; }
  IPAddress localIP() const { return ip; }
};
extern FakeWiFi WiFi;
