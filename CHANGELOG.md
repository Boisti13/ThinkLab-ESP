# Changelog

All notable changes, newest first. Each version is tagged `vX.Y.Z` on `main`.

## Unreleased

## v0.3.2 — 2026-11-15

### Fixed
- **Auto fan control could not start the fan.** The dead zone set the smoothed value back to 0 every 200 ms tick, and from 0 the smoothing only climbs about 0.5 % per tick, so below 8 % it never got going: once stopped, the fan only came back when a long e-paper redraw stalled the loop while the curve asked for 32 % or more (around 41 °C). The smoothing now runs on the command alone, and on/off is decided separately.
- **No chatter at the low end**: the fan starts when the smoothed command reaches 10 % and stops below 6 %, and stays on or off for at least a minute each, instead of switching (with a full-power kick) every tick around 8 %.
- **Sensor errors are no temperature**: the DS18B20 library reports −127 °C when the sensor doesn't answer and 85 °C for a reading that never ran; both were taken as real (−127 meant fan off). They now count as "no reading", so the fan goes to its safe 40 % after 15 s.
- A long loop stall no longer makes the fan filter jump.

### Added
- `POST /api/fan/manual/<0..100>`: any manual duty (the page's buttons still use 25 and 50; numbers outside 0–100 get 400).
- `status.json`: `mac` (stable ID for Home Assistant), `fan_curve_pct` (what the curve asks for), `fan_on`.
- `tools/fansim/`: runs the real fan controller on a PC against a scripted temperature run.

## v0.3.1 — 2026-11-15

### Added
- **The screen follows the data**: in Touch mode the current page redraws with every answer from the host (before, only on a tap, so it could show hours-old numbers). The board asks every minute (was two).
- **Partial refresh**: redraws without the black-white flash; every 20th is a full refresh to clear ghosting.
- **Stale marker**: a black "!" in the header of every page when the host hasn't answered for three polls; Status shows the link age compactly (`Online 42s`, `Lost 25m`).
- **OTA rollback**: a new build is only kept once it gets Wi-Fi; if it doesn't within 5 minutes (or crashes before), the board restarts into the previous build. Tested with a build that had a wrong Wi-Fi password.
- **Manual fan mode ends by itself** after an hour, or as soon as the case reaches 50 °C; the web page shows the minutes left.
- `status.json` also reports the host link (`host_age_s`, `host_stale`, `parse_err`), the fan (`fan_auto`, `fan_pct`, `fan_rpm`, `fan_manual_left_s`) and the image state (`image`). `/api/fan/status` adds `manual_left_s`.
- `homeassistant/thinklab_display.yaml`: Home Assistant package (REST sensors, fan scripts, alerts).

### Changed
- **VMs/LXCs page**: running guests first, then stopped ones.
- **Disks page**: no temperature column (the host never reads it, so as not to wake disks).
- JSON memory 24 KB (was 9 KB; a reply that didn't fit was dropped silently).

### Fixed
- Status page: the link line ran over its label when it said *Timeout*; the footer drew `[TOUCH]` twice.

### Under the hood
- Removed the empty `experimental` module, an unused template in `page_vms.cpp`, settings defined twice in `config.h`; `BUILD_VERSION` now follows `FW_VERSION`. All redraws go through `DisplayManager`.

## v0.3.0 — 2026-11-15

### Fixed
- **Wi-Fi**: one reconnect path instead of two. The main loop restarted Wi-Fi every 3 s while it was down, on top of `wifi_ota.cpp`'s own retries, so any connection slower than 3 s was cut off ("sta is connecting" errors). Now the driver's auto-reconnect handles drops, and Wi-Fi is restarted from scratch only after a minute without a connection.
- **Wi-Fi**: no more internet check. The board turned Wi-Fi off whenever `google.com` didn't resolve, so the web page and OTA were gone whenever the internet was (it only needs the LAN).
- **OTA**: ArduinoOTA's socket is closed and reopened properly across reconnects, and the board announces itself for OTA over mDNS again.
- **Web page**: *Exit Debug Page* shows the normal page at once again (lost in the 2025-11-30 build).

### Changed
- Firmware version 0.3.0 everywhere (the web page showed *Build v1.0.0*); expects host 1.2.0.

### Added
- Source of the build running on the author's board since 2025-11-30, which had never been committed: **fan buttons** on the web page (25 %, 50 %, back to auto; `/api/fan/status`, `/api/fan/manual/25|50`, `/api/fan/auto`), **Reboot ESP** (`/api/esp/reboot`), the host can restart the board with `{"schema_version":1,"cmd":"reboot"}`, a Wi-Fi check in the main loop every 3 s, and a simpler fan controller (8 % dead zone, start kick on every 0 → on edge, no hysteresis/dwell).
- Pictures of every e-paper page and of the web page in the README. `tools/render/` draws the pages on a PC from the real page code with made-up data, and takes the web page from a running board with every value replaced.

### Fixed
- **Network page**: the Wi-Fi line was drawn over the *Up* rate (the line position didn't move on after it).
- **VMs/LXCs page**: "+N more" counted only the six guests the board keeps, so with 15 containers it said "+3 more" instead of "+12 more"; it now counts from the host's total.

### Changed
- The OTA environment (`esp32c3-ota`) takes the board's address and password from `secrets.ini` (`ota_host`, `ota_pass`); they were written into `platformio.ini`. An existing `secrets.ini` needs those two lines (see `secrets.ini.example`).

### Under the hood
- `platformio.ini` pins `espressif32@6.12.0` (Arduino core 2.0.17): the unpinned platform now pulls Arduino core 3.x, where `ledcSetup`/`ledcAttachPin` are gone and the build failed.
- `CLAUDE.md` with the working rules for this repo; README rewritten; GitHub check that builds the firmware; `THIRD_PARTY_NOTICES.md` cleaned up.

## v0.2.3 — 2025-09-05

### Added
- Web UI:
  - ESP32 card now shows **Case temperature (Dallas sensor)**, same source as E-Ink Debug page.
  - `/status.json` now exposes `"case_c"` (null if unavailable).

### Changed
- All HTML pages now sent with explicit `UTF-8` charset to ensure correct rendering.
- Degree symbol is encoded as `&deg;C` to avoid garbled characters (e.g. “Â°C”).

### Fixed
- Temperature display on the web dashboard no longer shows stray “Â” before the degree symbol.

## v0.2.2 — 2025-08-29

### Added
- Web UI:
  - **Update Page** button that mirrors a single tap (re-renders current page and arms advance window).
  - **Next Page** button to advance to the next normal page and redraw immediately.
  - Endpoints: **`/api/ui/page/update`** and **`/api/ui/page/next`**.
  - Modern button styling utilities (`.btn`, `.btn-ghost`).

### Changed
- **Exit Debug Page** now cleanly leaves Debug mode and **immediately renders** the normal page (no simulated double-tap).
- All action buttons now use the **`btn btn-ghost`** style.
- Visibility: **Update Page** and **Next Page** are shown only in **TOUCH** mode and when **not** in Debug.

### Fixed
- Eliminated blank page caused by an unclosed `<style>` / improperly concatenated raw CSS blocks.

## v0.2.1 — 2025-08-26

### Added
- Website:
  - Favicon with transparent background (T red, L black, larger font).
  - `<title>` alias set to **ThinkLab Dash** for browser tab.
  - ESP uptime display (`Xd Yh Zm`) in ESP32 card.
  - Proxmox uptime display (`Xd Yh Zm`) in Proxmox card.
  - Proxmox disk overview showing each disk as active/idle square.
  - Disk legend row explaining icons (■ active / □ idle).

### Changed
- Website layout polished for consistency with e-ink Overview/Disk pages.
- Minor CSS cleanup for disk icons and alignment.

### Fixed
- Unified uptime formatting (days, hours, minutes) across ESP and host cards.

## v0.2.0 — 2025-08-26
### Added
- **Wi-Fi module**
  - Automatic connection to configured SSID.
  - Connection watchdog: if DNS not resolvable, Wi-Fi is disabled, waits 30 s, then retries.
  - Footer icon shows live RSSI as 0–4 bars, consistent thresholds (-55, -65, -75, -85 dBm).

- **OTA (Over-the-Air updates)**
  - ArduinoOTA integrated for remote firmware upload.
  - Protects against double init, stable across serial monitor reconnects.
  - Debug serial prints removed (silent by default).

- **Minimal Webserver**
  - Async HTTP server at `http://<esp_ip>/`.
  - Serves a status page with current Wi-Fi info and basic hostmon data.
  - Placeholder for future JSON API endpoints.

### Changed
- Refactored Wi-Fi reconnect handling to avoid crashes when serial monitor disconnects/reconnects.
- Adjusted Wi-Fi RSSI symbol placement (-5 px X, +4 px Y offset).

## v0.1.9 — 2025-08-15
### Added
- **Fan controller hysteresis & dwell knobs**:
  - `FAN_OFF_BELOW_PCT` (default **8**) – strictly below this → **0% (OFF)**
  - `FAN_ON_ABOVE_PCT` (default **12**) – must reach this to turn **ON** from OFF
  - `FAN_MIN_ON_MS` / `FAN_MIN_OFF_MS` (default **4000 ms**) – minimum dwell to prevent flapping

### Changed
- **Fan controller (`fanctrl`)**:
  - Simplified, predictable behavior:
    - **OFF below 32 °C**, **8% at 32 °C** (spin‑guarantee), aggressive ramp **≥ 40 °C**, **100% at 55 °C** (via `FC_PT*_C/P` flags).
  - Stability: gentle **rate limit** (`FAN_RATE_PCT_PER_S`) and **EMA smoothing** (`FAN_ALPHA_Q8`).
  - Start reliability: **one‑shot 250 ms @ 100%** start‑kick on OFF→ON.
  - Flap‑free control: **hysteresis + dwell**; while ON, duty never drops below `FAN_OFF_BELOW_PCT`.
  - Uses existing `fan1PwmSetPercent(...)` and publishes `fan_duty_cmd`, `fan_duty_filt`, `fan_active` for UI.

- **Debug page**:
  - Fan telemetry lines (`FanCmd`, `FanOut`, `FanAct`) aligned with existing layout using `labelX` / `valueR`.

## v0.1.8 — 2025-08-15
### Changed
- **Overview page**:
  - Strips CIDR suffix from displayed IP address during rendering  
    (e.g., `192.0.2.10/24` → `192.0.2.10`).
  - Kept `"Case:"` label for server internal/case temperature (unchanged from previous behavior).
  - No parser changes; render-only tweak in `page_overview.cpp`.
  - Preserves existing formats:
    - Right-aligned values
    - CPU % with one decimal
    - RAM `used/total GiB`
    - Uptime `Xd Yh`

## v0.1.7 — 2025-08-15
### Added
- **Fan controller module** (`fanctrl`):
  - Dallas temperature–based PWM control for `FAN1` using a piecewise-linear curve.
  - Soft start-kick, smoothing, and rate limiting to reduce noise and wear.
  - Fallback safe speed if Dallas sensor data is invalid for more than 15s.
  - Configurable curve points and behavior via `platformio.ini` build flags.
  - Stores `fan_duty_cmd`, `fan_duty_filt`, and `fan_active` in `HostState` for UI display.
- **Debug Page – Fan Visibility Controls**:
  - Added compile-time switches to enable/disable individual fan information lines:
    - `DBG_SHOW_FAN1_PWM` – show/hide Fan1 PWM %
    - `DBG_SHOW_FAN1_RPM` – show/hide Fan1 RPM
    - `DBG_SHOW_FAN2_RPM` – show/hide Fan2 RPM
    - `DBG_SHOW_FAN_BLOCK` – master toggle for `FanCmd`, `FanOut`, and `FanAct`
    - `DBG_SHOW_FAN_CMD` / `DBG_SHOW_FAN_OUT` / `DBG_SHOW_FAN_ACT` – fine-grained toggles inside block
  - Defaults in `config.h`, overrideable in `platformio.ini` via `build_flags`.
  - Defaults preserve previous behavior (all fan lines visible).

### Changed
- **Debug page**:
  - Added Fan1 telemetry:
    - Live commanded duty (`FanCmd`)
    - Filtered/applied duty (`FanOut`)
    - Fan active status (`FanAct`)
    - Existing Fan1 PWM % and RPM display preserved.
  - Fixed layout alignment by replacing undefined `L` with `labelX` for new Fan block.

### Fixed
- **Serial robustness**:
  - Accepts only `schema_version == 1`.
  - Drops explicit host error frames (e.g., `{"error":"unknown_command",...}`).
  - Non-JSON chatter already ignored by the brace-framed reader.

## v0.1.6 — 2025-08-14
### Added
- `DEBUG_IN_ROTATION` flag to control whether the Debug page appears in normal page rotation.
- Double-tap gesture to enter/exit dedicated Debug mode (Debug page refreshes on its own timer in this mode).

### Changed
- Refactored `DisplayManager`:
  - Fixed GxEPD2 display constructor to match current API.
  - Changed `display()` method to return explicit reference type (removed `auto` deduction dependency on C++14).
  - Updated page title functions to return `__FlashStringHelper*` for consistency.
- Adjusted touch button handling logic:
  - Single-tap refreshes current page, second tap (within threshold) advances to next page.
  - Long-press toggles Touch ↔ Auto page rotation modes.

### Fixed
- Splash screen no longer blocks until touch input.
- Build errors from incompatible return types in page title functions.
- Removed Debug page from rotation unless explicitly enabled via `DEBUG_IN_ROTATION`.

## v0.1.5 — 2025-08-14
### Added
- Fan1 PWM + tachometer module (`fan1_pwm`, `fan1_tach`)
- Fan2 tachometer module (`fan2_tach`)
- Experimental fan test sweep/fixed duty in `experimental` module

### Changed
- Unified fan config macros to `FAN1_*` and `FAN2_*` with defaults in `config.h`
- Debug page spacing reduced to `LINE_H=15` for more visible lines
- All fan/experimental includes now `#ifdef` guarded for `platformio.ini` control

## v0.1.4 — 2025-08-13
### Added
- Debug page JSON length display
- RAM usage display on Overview (Used/Total GiB)
- Right-aligned data points on Overview & Debug pages
- Serial connection status with timeout alert

### Changed
- Minor layout adjustments for page headers and spacing
