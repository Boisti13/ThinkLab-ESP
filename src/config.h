#pragma once

// Versions / compat
// Versions / compat (config.h)
#ifndef FW_NAME
  #define FW_NAME "hl-hostmon-esp"
#endif
#ifndef FW_VERSION
  #define FW_VERSION "0.1.1"
#endif
#ifndef SCHEMA_COMPAT
  #define SCHEMA_COMPAT 1
#endif
#ifndef HOST_SCRIPT_COMPAT
  #define HOST_SCRIPT_COMPAT "1.1.15"
#endif
#ifndef BUILD_TIMESTAMP
  #define BUILD_TIMESTAMP __DATE__ " " __TIME__
#endif


// Feature toggles
#ifndef USE_WIFI
#define USE_WIFI 0
#endif
#ifndef USE_DALLAS
#define USE_DALLAS 0
#endif
#ifndef USE_FANCTRL
#define USE_FANCTRL 0
#endif

// Pins
#define PIN_EPD_CS   7
#define PIN_EPD_DC   1
#define PIN_EPD_RST  2
#define PIN_EPD_BUSY 3
#define PIN_TOUCH    10
#define PIN_ONEWIRE  8
#define PIN_FAN_PWM  9
#define PIN_FAN_TACH 5

// Timings (ms)
#define POLL_INTERVAL_MS     2000
#define DISPLAY_INTERVAL_MS  5000
#define PAGE_DWELL_MS        5000
#define DEBOUNCE_MS          30
#define LONG_PRESS_MS        1200
#define VERY_LONG_PRESS_MS   3500

// Buffers
#define RX_LINEBUF_BYTES     12288
#define JSON_DOC_BYTES       24576

// Host timeout
#ifndef LINK_TIMEOUT_S
#define LINK_TIMEOUT_S 30   // consider the host "offline" if no JSON within this many seconds
#endif
