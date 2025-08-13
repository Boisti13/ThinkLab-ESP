#pragma once
#include "state.h"
#include "config.h"

class SerialClient {
public:
  void begin();                      // sends INFO once
  void tick(HostState& host, UiState& ui);  // polls GET and parses

private:
  char     buf[RX_LINEBUF_BYTES];
  size_t   n = 0;
  int      depth = 0;
  bool     inObj = false;
  uint32_t lastPollMs = 0;
  bool     infoSent = false;

  void sendINFO() { Serial.println("INFO"); }
  void sendGET()  { Serial.println("GET");  }

  bool parseAndStore(const char* json, size_t len, HostState& host, UiState& ui);
};
