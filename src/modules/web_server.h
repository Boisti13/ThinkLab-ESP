#pragma once
#include <Arduino.h>
#include <WebServer.h>

// Single server instance owned by this module
extern WebServer server;

// Init once from setup()
void webServerSetup();

// Call each loop()
void webServerLoop();

// OTA coordination (provided by wifi_ota.*)
void wifiOta_SetInUpload(bool on);
void wifiOta_RequestReboot();
