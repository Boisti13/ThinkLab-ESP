# ThinkLab — ESP32 client for hl-hostmon (e-ink dashboard)

Firmware for ESP32-C3/S3 that connects via **USB CDC** to the `hl-hostmon` host script,
polls JSON at fixed intervals, and renders a compact dashboard on a **1.54″ e-ink**
(SSD1681, 200×200) using **GxEPD2**. It supports touch/auto page modes, Dallas (case)
temperature, optional Wi-Fi/OTA, and leaves room for a future PC fan PWM controller.

> **Current firmware:** see `FW_VERSION` (set in `platformio.ini` → `build_flags`)

---

## Features

- **USB CDC client**: sends `INFO` once, then `GET` at a configurable interval
- **Robust JSON framing** (string/escape-aware) + ArduinoJson parsing
- **Consistent UI** with centered headers and right-aligned values
- **Pages**: Overview · Network · VMs/LXCs · Disks · Debug
- **Touch / Auto** page rotation, configurable poll interval
- **Dallas** 1-Wire temperature (“Case”)
- **Wi-Fi / OTA** optional (ArduinoOTA from Arduino IDE)
- **Future**: PC fan PWM on GPIO9 with tacho on GPIO5 (stubbed)

---

## Hardware

- **MCU**: ESP32-C3 (native USB) or ESP32-S3 (works similarly)
- **Display**: 1.54″ e-ink, SSD1681, 200×200  
  GxEPD2 model: `GxEPD2_154_D67`
- **Display wiring** (as used in code):

  | Signal | ESP32 pin |
  |--------|-----------|
  | CS     | 7         |
  | DC     | 1         |
  | RST    | 2         |
  | BUSY   | 3         |

  SPI uses the board defaults (no explicit remap needed).

- **Touch**: GPIO10 (active-low button or touch input)
- **Dallas**: 1-Wire pin configurable (see `config.h`)
- **Fan (planned)**: PWM on GPIO9, Tacho on GPIO5

---

## Build (PlatformIO)

This project is organized for **PlatformIO**. Example environment for ESP32-C3 USB:

```ini
; platformio.ini (excerpt)
[env:esp32c3-usb]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino

build_flags =
  -D FW_VERSION="0.1.4"
  -D POLL_INTERVAL_MS=2000      ; request interval (ms)
  -D LINK_TIMEOUT_S=30          ; "Online/Timeout" threshold on Overview (s)
  -D RX_LINEBUF_BYTES=8192      ; serial framing buffer (one JSON object)
  -D JSON_DOC_CAP=9216          ; ArduinoJson document capacity
  ; -D USE_WIFI=1               ; optional Wi-Fi
  ; -D USE_OTA=1                ; optional OTA (Arduino IDE)

lib_deps =
  bblanchon/ArduinoJson
  ZinggJM/GxEPD2
  adafruit/Adafruit GFX Library
  milesburton/DallasTemperature
```

**Build / Upload**

- USB upload (default): `PlatformIO: Upload` (env `esp32c3-usb`)
- OTA (optional): enable `USE_WIFI` and `USE_OTA`, set credentials in `config.h`,
  then use **Arduino IDE → Network Ports** to upload.

---

## Configuration

Most knobs are compile-time flags (see `build_flags`) with `config.h` fallbacks:

- `POLL_INTERVAL_MS` — how often to send `GET` to the host
- `LINK_TIMEOUT_S` — Overview shows **Online/Timeout** based on last JSON age
- `RX_LINEBUF_BYTES` — serial framing buffer (increase for larger payloads)
- `JSON_DOC_CAP` — ArduinoJson capacity (payload-dependent)
- `USE_WIFI`, `USE_OTA` — enable optional Wi-Fi/OTA

Runtime toggles live on the **Debug** page (e.g., debug flag) or via touch/auto mode.

---

## Pages

All pages share a centered, bold header with an underline and **right-aligned values**.

### Overview
- **IP** (primary IPv4)
- **CPU** — percent if available; fallback `L1 <load1>` when `cpu.percent` is `null`
- **RAM** — `used/total GiB` (binary GiB)
- **Case** — local Dallas temperature
- **Uptime** — `Xd Yh`
- **Link** — `Online (Xs)` or `Timeout (Xs)` based on last JSON age

### Network
- **IP**
- **Status** (e.g., `ok`)
- **Down** / **Up** — rates from `net` totals (`kbps`/`Mbps`), split across two lines

### VMs/LXCs
- **VMs:** `running/total`
- **LXCs:** `running/total`
- Short lists (up to 2 each): `- name` on the left, **status icon** on the right  
  (filled square = running, outline = stopped). Names are ellipsis-truncated to fit.

### Disks
- `- name` on the left
- **Temp** right-aligned (10 px left of the icon), `-` if unknown
- **Activity icon** on the far right (filled square = active, outline = idle)

### Debug
- **FW** version
- **RX age**
- **OK/ERR**
- **JSON len** (bytes)
- **Poll** interval (s)
- **Mode** (TOUCH/AUTO)
- **Debug** flag

---

## Host protocol (what we expect)

The device writes:
- `INFO
` once on boot
- `GET
` every `POLL_INTERVAL_MS`

The host replies with a single **JSON object** per line (or with newlines—we frame by braces).

**Fields used** (examples, not exhaustive):

```json
{
  "uptime_s": 181284,
  "hostname": "bkg-LPS",
  "cpu": { "percent": null, "load1": 0.49, "load5": 0.17, "load15": 0.11 },
  "ram": { "total_bytes": 16513433600, "used_bytes": 3926523904 },
  "filesystems": [{ "mount": "/", "total_bytes": 100861726720, "used_bytes": 14402846720 }],
  "proxmox": {
    "vm_running": 1, "vm_total": 4,
    "lxc_running": 0, "lxc_total": 0,
    "vms":  [{ "id": 100, "name": "UbuntuLTS24.04", "status": "running" }],
    "lxcs": [{ "id": 201, "name": "dns", "status": "stopped" }]
  },
  "disks": [{ "name": "nvme0n1", "state": "active", "temp_C": null }],
  "ip": {
    "primary_ifname": "vmbr2",
    "primary_ipv4": "192.168.100.103",
    "gateway_ipv4": "192.168.100.1",
    "ip_status": "ok"
  },
  "net": {
    "window_s": 1,
    "total_rx_bps": 83216, "total_tx_bps": 83240
  }
}
```

Notes:
- `cpu.percent` may be `null`; we fall back to `cpu.load1`.
- `ram.*_bytes` are parsed as 64-bit and shown as **GiB**.
- `disks[*].state`: `"active"` / `"active/idle"` → active icon; temp `null` → “-”.
- `net.total_rx_bps/total_tx_bps` preferred; we also accept `*_Bps` and per-interface fallback.

---

## Development

- **Structure**: modular `src/pages/*` and `src/modules/*` (Wi-Fi, Dallas, etc.)
- **UI helpers**: `ui_theme.h` provides `header(...)`, `content_top()`, and `ui::printRight(...)`
- **No serial spam**: Serial is the host link; all debug info goes to the **Debug** page
- **Extending pages**: Prefer right-aligned values; measure and fit text to avoid collisions

---

## Troubleshooting

- **CPU shows “-”**: host sent `"cpu":{"percent":null}` and no loads; add `load1` or compute percent on host.
- **No Disks**: ensure `disks` array present; temp may be `null` (we show “-”).
- **Time-outs**: Overview `Link` uses `LINK_TIMEOUT_S`; bump if host polls slower.
- **JSON too large**: check Debug → **JSON len** vs `RX_LINEBUF_BYTES`. Increase buffers if needed.

---

## Versioning & Changelog

We use semver-ish tags (`vX.Y.Z`). See [`CHANGELOG.md`](CHANGELOG.md) for details.

---

## License

This project is licensed under **GPL-3.0-or-later**. See [`LICENSE`](LICENSE) and
[`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md).

Libraries:
- GxEPD2 (GPL-3.0), Adafruit GFX (BSD), ArduinoJson (MIT), DallasTemperature (MIT).

---

## Acknowledgements

- ZinggJM for **GxEPD2**
- Adafruit for **GFX**
- Benoît Blanchon for **ArduinoJson**
- Miles Burton et al. for **DallasTemperature**
- ThinkLab project contributors
