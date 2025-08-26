#pragma once
#include <Arduino.h>

void wifiOtaSetup();    // call once from setup()
void wifiOtaLoop();     // call each loop

// Exposed for web_server.cpp to coordinate uploads and safe reboot
void wifiOta_SetInUpload(bool on);
void wifiOta_RequestReboot();
