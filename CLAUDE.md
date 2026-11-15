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
- **The board's source is here since 2026-11-15**: the build it ran
  (2025-11-30, fan buttons, reboot) was only on a Windows PC, uncommitted;
  the owner uploaded it and it was committed on top. `secrets.ini` with the
  real values is in the dev box clone (git-ignored, mode 600), so real
  builds work there.
- **Versions**: the real one is `-DFW_VERSION` in `platformio.ini`'s
  `build_flags` (the default in `config.h` is only a fallback). Bump it
  with every flashed change and add a `CHANGELOG.md` entry;
  `HOST_SCRIPT_COMPAT` names the host version it expects.
- **Fan controller**: `tools/fansim/build.sh` runs `fanctrl.cpp` on the PC
  through a scripted temperature run; run it after every controller change
  and check boot, warm-up, the low-end edge (no chatter) and sensor loss.
- **Pictures**: `tools/render/build.sh` renders every e-paper page to
  `docs/screenshots/eink-*.png` (PC build of `src/pages/` against the
  stand-ins in `tools/render/shim/`, made-up data in `render.cpp`); run it
  after every visible page change and look at the PNGs. A page that uses a
  new Arduino/GxEPD2 call needs it added to the shim. The web picture comes
  from `tools/render/web-screenshot.js` against the live board (one GET,
  every value replaced); never commit real addresses, SSIDs or names.
- **Build** on the dev box with PlatformIO (venv `~/.venvs/platformio`):
  `pio run -e esp32c3-usb`. A build needs a `secrets.ini`; for a compile
  check copy `secrets.ini.example` (dummy values) and never flash that.
  The GitHub checks (`checks.yml`) build it that way and render the pages.
  The platform stays pinned (`espressif32@6.12.0`): newer ones ship
  Arduino core 3.x, which this code doesn't build with.
- **Flashing is the owner's call**: the board is plugged into a Proxmox
  host, so USB flashing means physical access or passing the device
  through; over the network it is the web UI's `/update` page or
  ArduinoOTA (`esp32c3-ota` env, address and password from `secrets.ini`).
  As a fallback the host it is plugged into can flash it over USB with
  `esptool` (stop `hl-hostmon` first, it holds the port). Ask before
  flashing.
- Budget: the C3 has no PSRAM. Keep the JSON document size
  (`JSON_DOC_CAP` in `serial_client.cpp`) above the real payload with
  headroom — a payload that doesn't fit is dropped silently
  (`parseErrCount`) and the screen goes stale.
- The display is e-paper and every redraw is a full refresh (no partial
  windows yet): don't redraw more often than needed.
- README and CHANGELOG follow the owner's house style (badges, highlights,
  tables; `## vX.Y.Z — date`, Added / Changed / Fixed / Under the hood,
  *Unreleased* on top); update both with every user-visible change.
