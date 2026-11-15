# Third-party notices

The firmware is built with these libraries; each stays under its own licence.

| Library | Licence | Source |
|---|---|---|
| GxEPD2 | GPL-3.0 | <https://github.com/ZinggJM/GxEPD2> |
| Adafruit GFX Library (incl. its fonts) | BSD-2-Clause | <https://github.com/adafruit/Adafruit-GFX-Library> |
| ArduinoJson | MIT | <https://github.com/bblanchon/ArduinoJson> |
| DallasTemperature | MIT | <https://github.com/milesburton/Arduino-Temperature-Control-Library> |
| OneWire | MIT-style (see the source headers) | <https://github.com/PaulStoffregen/OneWire> |
| Arduino core for ESP32 (WiFi, ESPmDNS, ArduinoOTA, WebServer, Update) | LGPL-2.1 | <https://github.com/espressif/arduino-esp32> |

`tools/render/` compiles the page code against Adafruit GFX on a PC; it
downloads the library at build time and doesn't ship it.
