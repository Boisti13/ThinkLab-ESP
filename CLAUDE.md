# ThinkLab-ESP — working rules

Firmware (PlatformIO, Arduino framework) for the ThinkLab e-ink display:
an ESP32-C3 with a 1.54" SSD1681 e-paper (GxEPD2), a touch button, a
Dallas case sensor, Wi-Fi with a small web UI and OTA, and fan PWM/tach
code. It asks the host service from the sibling repo **ThinkLab-Host**
for a JSON snapshot over USB CDC (`GET`, every `POLL_INTERVAL_MS`) and
renders pages (Overview, Network, VMs/LXCs, Disks, Debug). Schema changes
go into both repos together.

- **Public repo (GPL-3.0).** Never commit `secrets.ini` (Wi-Fi, web/OTA
  login, hostname; it is git-ignored, `secrets.ini.example` shows the
  keys) or the owner's addresses, MACs or device paths. Where the board
  sits and how to reach it is in the owner's private notes
  (`claude-setup/homelab.md`, wiki).
- One branch, `main`; the old feature/release branches on GitHub are
  merged or abandoned — don't build on them.
- **Versions**: the real one is `-DFW_VERSION` in `platformio.ini`'s
  `build_flags` (the default in `config.h` is only a fallback). Bump it
  with every flashed change and add a `CHANGELOG.md` entry;
  `HOST_SCRIPT_COMPAT` names the host version it expects.
- **Build** on the dev box with PlatformIO (venv `~/.venvs/platformio`):
  `pio run -e esp32c3-usb`. A build needs a `secrets.ini`; for a compile
  check copy `secrets.ini.example` (dummy values) and never flash that.
- **Flashing is the owner's call**: the board is plugged into a Proxmox
  host, so USB flashing means physical access or passing the device
  through; over the network it is the web UI's `/update` page or
  ArduinoOTA (`esp32c3-ota` env). Ask before flashing.
- Budget: the C3 has no PSRAM. Keep the JSON document size
  (`JSON_DOC_CAP` in `serial_client.cpp`) above the real payload with
  headroom — a payload that doesn't fit is dropped silently
  (`parseErrCount`) and the screen goes stale.
- The display is e-paper and every redraw is a full refresh (no partial
  windows yet): don't redraw more often than needed.
- README and CHANGELOG follow the owner's house style (badges, highlights,
  tables; `## vX.Y.Z — date`, Added / Changed / Fixed / Under the hood,
  *Unreleased* on top); update both with every user-visible change.
