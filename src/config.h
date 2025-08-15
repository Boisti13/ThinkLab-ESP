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

// Pins
#define PIN_EPD_CS   7
#define PIN_EPD_DC   1
#define PIN_EPD_RST  2
#define PIN_EPD_BUSY 3
#define PIN_ONEWIRE  8

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
  #define TOUCH_LONG_MS 900     // long-press threshold
#endif
#ifndef TOUCH_DBL_MS
  #define TOUCH_DBL_MS 350      // double-tap window
#endif
#ifndef TOUCH_ADVANCE_ARM_MS
  #define TOUCH_ADVANCE_ARM_MS 20000  // 20s: second press advances page
#endif

// ===== Debug mode refresh =====
#ifndef DEBUG_REFRESH_MS
  #define DEBUG_REFRESH_MS 5000
#endif

// Debug page participation in normal rotation (0 = only via double‑tap; 1 = included)
#ifndef DEBUG_IN_ROTATION
  #define DEBUG_IN_ROTATION 0
#endif


// Timings (ms)
#define POLL_INTERVAL_MS     60000
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
#define LINK_TIMEOUT_S 600   // consider the host "offline" if no JSON within this many seconds
#endif

// ===================== Fan 1 (PWM + tach) =====================
#ifndef USE_FAN1
  #define USE_FAN1 1                 // 0 => compile out fan1 modules
#endif
#ifndef FAN1_PWM_PIN
  #define FAN1_PWM_PIN 9             // GPIO9 → transistor → fan PWM (open-collector)
#endif
#ifndef FAN1_PWM_FREQ_HZ
  #define FAN1_PWM_FREQ_HZ 25000     // Intel 4-wire spec
#endif
#ifndef FAN1_DEFAULT_PWM_PCT
  #define FAN1_DEFAULT_PWM_PCT 50    // startup duty (%)
#endif
#ifndef FAN1_MIN_DUTY_PCT
  #define FAN1_MIN_DUTY_PCT 8        // Arctic P8 Slim: ≤5% → 0 rpm; margin
#endif

#ifndef FAN1_TACH_PIN
  #define FAN1_TACH_PIN 5            // GPIO5 (tach input, pull-up to 3.3V)
#endif
#ifndef FAN1_PPR
  #define FAN1_PPR 2                 // pulses per revolution
#endif
#ifndef FAN1_TACH_WIN_MS
  #define FAN1_TACH_WIN_MS 500       // measurement window
#endif
#ifndef FAN1_TACH_TIMEOUT_MS
  #define FAN1_TACH_TIMEOUT_MS 2000  // no pulses → 0 RPM
#endif
#ifndef FAN1_PWM_SET
  #define FAN1_PWM_SET(pct) fan1PwmSet(pct)
#endif

// ===================== Fan 2 (tach-only) ======================
#ifndef USE_FAN2_TACH
  #define USE_FAN2_TACH 1            // 0 => compile out fan2 tach module
#endif
#ifndef FAN2_TACH_PIN
  #define FAN2_TACH_PIN 0            // GPIO0 (strap pin) — INPUT_PULLUP is safe
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
#ifndef USE_EXPERIMENTAL
  #define USE_EXPERIMENTAL 1         // 0 => compile out all experiments
#endif

// Fixed PWM test for Fan 1 (applied once at boot if sweep is OFF)
#ifndef EXP_TEST_FAN1_PWM
  #define EXP_TEST_FAN1_PWM 0
#endif
#ifndef FAN1_TEST_PWM_PCT
  #define FAN1_TEST_PWM_PCT 60       // 0..100
#endif

// Fan 1 PWM sweep test (triangle) — overrides fixed test while enabled
#ifndef EXP_TEST_FAN1_SWEEP
  #define EXP_TEST_FAN1_SWEEP 1
#endif
#ifndef FAN1_SWEEP_MIN_PCT
  #define FAN1_SWEEP_MIN_PCT 20
#endif
#ifndef FAN1_SWEEP_MAX_PCT
  #define FAN1_SWEEP_MAX_PCT 100
#endif
#ifndef FAN1_SWEEP_PERIOD_MS
  #define FAN1_SWEEP_PERIOD_MS 30000
#endif

// ==== Fan control (Dallas-only) ====
#ifndef USE_FANCTRL
  #define USE_FANCTRL 1
#endif

#ifndef FAN_OFF_BELOW_PCT
  #define FAN_OFF_BELOW_PCT 5      // commanded < 5% -> 0%
#endif
#ifndef FAN_MIN_DUTY_PCT
  #define FAN_MIN_DUTY_PCT 8       // reliable minimum
#endif
#ifndef FAN_RATE_PCT_PER_S
  #define FAN_RATE_PCT_PER_S 10    // slope limiter (up/down)
#endif
#ifndef FAN_ALPHA_Q8
  #define FAN_ALPHA_Q8 64          // 64/256 = 0.25 smoothing
#endif
#ifndef FAN_KICK_MS
  #define FAN_KICK_MS 250
#endif
#ifndef FAN_KICK_PCT
  #define FAN_KICK_PCT 100
#endif
#ifndef FAN_INVALID_HOLD_MS
  #define FAN_INVALID_HOLD_MS 15000  // after this, go safe
#endif
#ifndef FAN_SAFE_PCT
  #define FAN_SAFE_PCT 40
#endif
#ifndef FAN_TICK_MS
  #define FAN_TICK_MS 200          // 5 Hz
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
  #define DBG_SHOW_FAN_BLOCK 1  // master switch for FanCmd/FanOut/FanAct
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
