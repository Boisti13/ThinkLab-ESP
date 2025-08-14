# Changelog

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
