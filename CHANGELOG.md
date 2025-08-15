# Changelog

## [0.1.7] - 2025-08-15
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

## [0.1.6] - 2025-08-14
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

---

## [0.1.5] - 2025-08-14
### Added
- Fan1 PWM + tachometer module (`fan1_pwm`, `fan1_tach`)
- Fan2 tachometer module (`fan2_tach`)
- Experimental fan test sweep/fixed duty in `experimental` module

### Changed
- Unified fan config macros to `FAN1_*` and `FAN2_*` with defaults in `config.h`
- Debug page spacing reduced to `LINE_H=15` for more visible lines
- All fan/experimental includes now `#ifdef` guarded for `platformio.ini` control

---

## [0.1.4] - 2025-08-13
### Added
- Debug page JSON length display
- RAM usage display on Overview (Used/Total GiB)
- Right-aligned data points on Overview & Debug pages
- Serial connection status with timeout alert

### Changed
- Minor layout adjustments for page headers and spacing
