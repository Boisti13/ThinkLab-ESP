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
#ifndef BUILD_VERSION
#define BUILD_VERSION "v" FW_VERSION   // shown on the web page and in status.json
#endif

// Feature toggles
#ifndef USE_WIFI
#define USE_WIFI 0
#endif
#ifndef USE_DALLAS
#define USE_DALLAS 0
#endif

// Pins
#define PIN_EPD_CS 7
#define PIN_EPD_DC 1
#define PIN_EPD_RST 2
#define PIN_EPD_BUSY 3
#define PIN_ONEWIRE 8

// ===== Touch / Button =====

// Button polarity: 1 = active LOW (with INPUT_PULLUP), 0 = active HIGH (with INPUT_PULLDOWN)
#ifndef TOUCH_ACTIVE_LOW
#define TOUCH_ACTIVE_LOW 0
#endif

#ifndef TOUCH_PIN
#define TOUCH_PIN 10
#endif
#ifndef TOUCH_DEBOUNCE_MS
#define TOUCH_DEBOUNCE_MS 30
#endif
#ifndef TOUCH_LONG_MS
#define TOUCH_LONG_MS 900 // long-press threshold
#endif
#ifndef TOUCH_DBL_MS
#define TOUCH_DBL_MS 500 // double-tap window (350)
#endif
#ifndef TOUCH_ADVANCE_ARM_MS
#define TOUCH_ADVANCE_ARM_MS 20000 // 20s: second press advances page
#endif

// ===== Debug mode refresh =====
#ifndef DEBUG_REFRESH_MS
#define DEBUG_REFRESH_MS 5000
#endif

// Debug page participation in normal rotation (0 = only via double‑tap; 1 = included)
#ifndef DEBUG_IN_ROTATION
#define DEBUG_IN_ROTATION 0
#endif

// Silence any debug in libs that rely on DEBUG/GxEPD2_DEBUG
//#ifdef DEBUG
//#undef DEBUG
//#endif
//#ifdef GxEPD2_DEBUG
//#undef GxEPD2_DEBUG
//#endif


// Timings (ms)
#ifndef POLL_INTERVAL_MS
#define POLL_INTERVAL_MS 60000      // ask the host this often; the page redraws with each answer
#endif
#ifndef DISPLAY_INTERVAL_MS
#define DISPLAY_INTERVAL_MS 5000    // AUTO mode: time per page
#endif

// Data older than this is marked on every page (header shows "!")
#ifndef STALE_AFTER_S
#define STALE_AFTER_S (3 * POLL_INTERVAL_MS / 1000)
#endif

// E-paper: partial refreshes in between, a full one every N redraws to clear ghosting
#ifndef FULL_REFRESH_EVERY
#define FULL_REFRESH_EVERY 20
#endif

// OTA rollback: a new build must get Wi-Fi within this time, or the board
// restarts and the bootloader goes back to the previous build.
#ifndef OTA_VERIFY_TIMEOUT_MS
#define OTA_VERIFY_TIMEOUT_MS 300000
#endif

// Buffers
#define RX_LINEBUF_BYTES 12288
#define JSON_DOC_BYTES 24576        // ArduinoJson memory for one answer (~5 KB needed for 16 guests)

// Host timeout
#ifndef LINK_TIMEOUT_S
#define LINK_TIMEOUT_S 600 // consider the host "offline" if no JSON within this many seconds
#endif

// ===================== Fan 1 (PWM + tach) =====================
#ifndef USE_FAN1
#define USE_FAN1 1 // 0 => compile out fan1 modules
#endif
#ifndef FAN1_PWM_PIN
#define FAN1_PWM_PIN 9 // GPIO9 → transistor → fan PWM (open-collector)
#endif
#ifndef FAN1_PWM_FREQ_HZ
#define FAN1_PWM_FREQ_HZ 25000 // Intel 4-wire spec
#endif
#ifndef FAN1_DEFAULT_PWM_PCT
#define FAN1_DEFAULT_PWM_PCT 50 // startup duty (%)
#endif
#ifndef FAN1_MIN_DUTY_PCT
#define FAN1_MIN_DUTY_PCT 8 // Arctic P8 Slim: ≤5% → 0 rpm; margin
#endif

#ifndef FAN1_TACH_PIN
#define FAN1_TACH_PIN 5 // GPIO5 (tach input, pull-up to 3.3V)
#endif
#ifndef FAN1_PPR
#define FAN1_PPR 2 // pulses per revolution
#endif
#ifndef FAN1_TACH_WIN_MS
#define FAN1_TACH_WIN_MS 500 // measurement window
#endif
#ifndef FAN1_TACH_TIMEOUT_MS
#define FAN1_TACH_TIMEOUT_MS 2000 // no pulses → 0 RPM
#endif
#ifndef FAN1_PWM_SET
#define FAN1_PWM_SET(pct) fan1PwmSet(pct)
#endif

// ===================== Fan 2 (tach-only) ======================
#ifndef USE_FAN2_TACH
#define USE_FAN2_TACH 1 // 0 => compile out fan2 tach module
#endif
#ifndef FAN2_TACH_PIN
#define FAN2_TACH_PIN 0 // GPIO0 (strap pin) — INPUT_PULLUP is safe
#endif
#ifndef FAN2_PPR
#define FAN2_PPR 2
#endif
#ifndef FAN2_TACH_WIN_MS
#define FAN2_TACH_WIN_MS 500
#endif
#ifndef FAN2_TACH_TIMEOUT_MS
#define FAN2_TACH_TIMEOUT_MS 2000
#endif

// ===================== Experimental ===========================


// ==== Fan control (Dallas-only) ====
#ifndef USE_FANCTRL
#define USE_FANCTRL 1
#endif

// On/off with hysteresis: the smoothed command must reach FAN_ON_ABOVE_PCT to
// start the fan and fall below FAN_OFF_BELOW_PCT to stop it, and each state
// is held at least FAN_MIN_ON_MS / FAN_MIN_OFF_MS.
#ifndef FAN_ON_ABOVE_PCT
#define FAN_ON_ABOVE_PCT 10
#endif
#ifndef FAN_OFF_BELOW_PCT
#define FAN_OFF_BELOW_PCT 6
#endif
#ifndef FAN_MIN_ON_MS
#define FAN_MIN_ON_MS 60000UL
#endif
#ifndef FAN_MIN_OFF_MS
#define FAN_MIN_OFF_MS 60000UL
#endif
#ifndef FAN_RATE_PCT_PER_S
#define FAN_RATE_PCT_PER_S 10 // slope limiter (up/down)
#endif
#ifndef FAN_ALPHA_Q8
#define FAN_ALPHA_Q8 64 // 64/256 = 0.25 smoothing
#endif
#ifndef FAN_KICK_MS
#define FAN_KICK_MS 250
#endif
#ifndef FAN_KICK_PCT
#define FAN_KICK_PCT 100
#endif
#ifndef FAN_INVALID_HOLD_MS
#define FAN_INVALID_HOLD_MS 15000 // after this, go safe
#endif
#ifndef FAN_SAFE_PCT
#define FAN_SAFE_PCT 40
#endif
#ifndef FAN_TICK_MS
#define FAN_TICK_MS 200 // 5 Hz
#endif
// Manual duty from the web page ends after this long, or as soon as the case
// reaches FAN_MANUAL_OVERRIDE_C, and the curve takes over again.
#ifndef FAN_MANUAL_MAX_MS
#define FAN_MANUAL_MAX_MS 3600000UL
#endif
#ifndef FAN_MANUAL_OVERRIDE_C
#define FAN_MANUAL_OVERRIDE_C 50
#endif

// Curve points (°C → duty %)
#ifndef FC_PT1_C
#define FC_PT1_C 35
#endif
#ifndef FC_PT1_P
#define FC_PT1_P 0
#endif
#ifndef FC_PT2_C
#define FC_PT2_C 40
#endif
#ifndef FC_PT2_P
#define FC_PT2_P 15
#endif
#ifndef FC_PT3_C
#define FC_PT3_C 45
#endif
#ifndef FC_PT3_P
#define FC_PT3_P 25
#endif
#ifndef FC_PT4_C
#define FC_PT4_C 50
#endif
#ifndef FC_PT4_P
#define FC_PT4_P 50
#endif
#ifndef FC_PT5_C
#define FC_PT5_C 55
#endif
#ifndef FC_PT5_P
#define FC_PT5_P 100
#endif

// ---- Debug page fan visibility (compile-time) ----
#ifndef DBG_SHOW_FAN1_PWM
#define DBG_SHOW_FAN1_PWM 1
#endif
#ifndef DBG_SHOW_FAN1_RPM
#define DBG_SHOW_FAN1_RPM 1
#endif
#ifndef DBG_SHOW_FAN2_RPM
#define DBG_SHOW_FAN2_RPM 1
#endif

#ifndef DBG_SHOW_FAN_BLOCK
#define DBG_SHOW_FAN_BLOCK 1 // master switch for FanCmd/FanOut/FanAct
#endif
#ifndef DBG_SHOW_FAN_CMD
#define DBG_SHOW_FAN_CMD 1
#endif
#ifndef DBG_SHOW_FAN_OUT
#define DBG_SHOW_FAN_OUT 1
#endif
#ifndef DBG_SHOW_FAN_ACT
#define DBG_SHOW_FAN_ACT 1
#endif

#ifndef DBG_SHOW_DALLAS
  #define DBG_SHOW_DALLAS 1
#endif

#ifndef DBG_SHOW_WIFI
  #define DBG_SHOW_WIFI 1
#endif

#ifndef DBG_SHOW_WIFI_RSSI
  #define DBG_SHOW_WIFI_RSSI 1
#endif
