# Changelog

# Changelog

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
