#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "state.h"
#include "serial_client.h"

// ---------- JSON doc capacity (can be overridden in config.h / build_flags) ----------
#ifndef JSON_DOC_CAP
  #define JSON_DOC_CAP 9216   // generous headroom for bigger payloads
#endif

static DynamicJsonDocument s_doc(JSON_DOC_CAP);

// Track quotes/escapes so braces inside strings don’t confuse the depth counter
static bool s_inString = false;
static bool s_esc      = false;

static inline void resetStringState() { s_inString = false; s_esc = false; }

static inline void safeCopy(char* dst, size_t dstSz, const char* src) {
  if (!dst || dstSz == 0) return;
  if (!src) { dst[0] = 0; return; }
  strncpy(dst, src, dstSz - 1);
  dst[dstSz - 1] = 0;
}

void SerialClient::begin() {
  n = 0;
  depth = 0;
  inObj = false;
  lastPollMs = 0;
  infoSent = false;
  resetStringState();
}

void SerialClient::tick(HostState& host, UiState& ui) {
  const uint32_t now = millis();

  // TX: INFO once, then GET at POLL_INTERVAL_MS
  if (!infoSent) {
    if (Serial) { sendINFO(); infoSent = true; lastPollMs = now; }
  } else if (now - lastPollMs >= POLL_INTERVAL_MS) {
    sendGET();
    lastPollMs = now;
  }

  // RX: brace-framed reader (tolerates newlines)
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;  // ignore CR

    if (!inObj) {
      if (c == '{') {
        inObj = true;
        depth = 1;
        n = 0;
        resetStringState();
        if (n < RX_LINEBUF_BYTES - 1) buf[n++] = c;
        else { ui.rxOverflowCnt++; inObj = false; n = 0; }
      }
      continue;
    }

    // track string/escape state
    if (s_inString) {
      if (s_esc) s_esc = false;
      else if (c == '\\') s_esc = true;
      else if (c == '"')  s_inString = false;
    } else {
      if (c == '"') s_inString = true;
      else if (c == '{') depth++;
      else if (c == '}') depth--;
    }

    // store with overflow guard
    if (n < RX_LINEBUF_BYTES - 1) buf[n++] = c;
    else { ui.rxOverflowCnt++; inObj = false; n = 0; depth = 0; resetStringState(); continue; }

    // end of object?
    if (!s_inString && depth == 0) {
      buf[n] = '\0';
      (void)parseAndStore(buf, n, host, ui);
      inObj = false; n = 0; depth = 0; resetStringState();
    }
  }
}

// Helper: map disk state string -> active flag
static bool stateIsActive(const char* st) {
  if (!st) return false;
  // normalize common values
  if (strcmp(st, "active") == 0)   return true;
  if (strcmp(st, "idle") == 0)     return false;
  if (strcmp(st, "standby") == 0)  return false;
  // Some tools emit numbers or other tokens; be conservative:
  // treat anything that equals "1" or "true" as active
  if (strcmp(st, "1") == 0)        return true;
  if (strcasecmp(st, "true") == 0) return true;
  return false;
}

// Helper: extract temperature; returns sentinel -127 if missing/null
static int8_t extractDiskTempC(JsonObjectConst d) {
  // accept a few key spellings
  if (!d["temp_C"].isNull()) {
    if (d["temp_C"].is<JsonInteger>()) return (int8_t)d["temp_C"].as<int>();
    if (d["temp_C"].is<float>())       return (int8_t)lroundf(d["temp_C"].as<float>());
    // null or non-numeric -> sentinel
    return -127;
  }
  if (!d["temp_c"].isNull()) {
    if (d["temp_c"].is<JsonInteger>()) return (int8_t)d["temp_c"].as<int>();
    if (d["temp_c"].is<float>())       return (int8_t)lroundf(d["temp_c"].as<float>());
    return -127;
  }
  if (!d["temperature_C"].isNull()) {
    if (d["temperature_C"].is<JsonInteger>()) return (int8_t)d["temperature_C"].as<int>();
    if (d["temperature_C"].is<float>())       return (int8_t)lroundf(d["temperature_C"].as<float>());
    return -127;
  }
  return -127;
}

bool SerialClient::parseAndStore(const char* json, size_t len, HostState& host, UiState& ui) {
  if (!json || len == 0) return false;

  ui.lastJsonLen = (uint16_t)len;

  s_doc.clear();
  DeserializationError err = deserializeJson(s_doc, json, len);
  if (err) { ui.parseErrCount++; return false; }

  JsonVariantConst root = s_doc.as<JsonVariantConst>();

  // --- accept only our schema; ignore other JSON or host chatter ---
  const int schema = root["schema_version"] | -1;
  if (schema != 1) {
    return false;
  }

  // --- ignore explicit error frames from the host ---
  if (!root["error"].isNull()) {
    return false;
  }

  // From here on, it’s a valid v1 payload we care about
  ui.parseOkCount++;
  ui.lastParseOkMs = millis();
  ui.firstDataReady = true;

  // Keep preview for Debug page (only for accepted payloads)
  host.last_json = String(json, len);

  // ---- top-level we use ----
  host.uptime_sec = root["uptime_s"] | 0;
  safeCopy(host.hostname, sizeof(host.hostname), root["hostname"] | "");

  // ---- CPU ----
  if (root["cpu"].is<JsonObjectConst>()) {
    JsonObjectConst c = root["cpu"];
    host.cpu_percent = c["percent"].isNull() ? NAN : c["percent"].as<float>();
    host.load1  = c["load1"].isNull()  ? NAN : c["load1"].as<float>();
    host.load5  = c["load5"].isNull()  ? NAN : c["load5"].as<float>();
    host.load15 = c["load15"].isNull() ? NAN : c["load15"].as<float>();
  } else {
    host.cpu_percent = NAN;
    host.load1 = host.load5 = host.load15 = NAN;
  }

  // ---- RAM (64-bit) ----
  host.ram_total = 0; host.ram_used = 0;
  if (root["ram"].is<JsonObjectConst>()) {
    JsonObjectConst r = root["ram"];
    if (r.containsKey("total_bytes")) host.ram_total = r["total_bytes"].as<uint64_t>();
    else if (r.containsKey("total"))  host.ram_total = r["total"].as<uint64_t>();

    if (r.containsKey("used_bytes"))  host.ram_used  = r["used_bytes"].as<uint64_t>();
    else if (r.containsKey("used"))   host.ram_used  = r["used"].as<uint64_t>();
  }

  // ---- Filesystems (root "/") ----
  host.fs_root_total = 0; host.fs_root_used = 0;
  if (root["filesystems"].is<JsonArrayConst>()) {
    for (JsonObjectConst fs : root["filesystems"].as<JsonArrayConst>()) {
      const char* m = fs["mount"] | "";
      if (m && strcmp(m, "/") == 0) {
        if (fs.containsKey("total_bytes")) host.fs_root_total = fs["total_bytes"].as<uint64_t>();
        if (fs.containsKey("used_bytes"))  host.fs_root_used  = fs["used_bytes"].as<uint64_t>();
        break;
      }
    }
  }

  // ---- Proxmox counts ----
  host.vms_running  = -1; host.vms_total = -1;
  host.lxcs_running = -1; host.lxcs_total = -1;
  if (root["proxmox"].is<JsonObjectConst>()) {
    JsonObjectConst p = root["proxmox"];
    if (!p["vm_running"].isNull())  host.vms_running  = p["vm_running"].as<int32_t>();
    if (!p["vm_total"].isNull())    host.vms_total    = p["vm_total"].as<int32_t>();
    if (!p["lxc_running"].isNull()) host.lxcs_running = p["lxc_running"].as<int32_t>();
    if (!p["lxc_total"].isNull())   host.lxcs_total   = p["lxc_total"].as<int32_t>();
  }

  // ---- Proxmox guest lists (names/status) ----
  host.vm_list_count  = 0;
  host.lxc_list_count = 0;
  if (root["proxmox"].is<JsonObjectConst>()) {
    JsonObjectConst p = root["proxmox"];

    if (p["vms"].is<JsonArrayConst>()) {
      const size_t CAP = sizeof(host.vm_list) / sizeof(host.vm_list[0]);
      for (JsonObjectConst v : p["vms"].as<JsonArrayConst>()) {
        if (host.vm_list_count >= CAP) break;
        auto &dst = host.vm_list[host.vm_list_count++];
        dst.id = v["id"].isNull() ? -1 : v["id"].as<int32_t>();
        const char* nm = v["name"] | "";
        strncpy(dst.name, nm, sizeof(dst.name) - 1);
        const char* st = v["status"] | "";
        // Some hosts send "running"/"stopped"; yours sends "0"/"1"
        dst.running = (st && (strcmp(st, "running") == 0 || strcmp(st, "1") == 0));
      }
    }

    if (p["lxcs"].is<JsonArrayConst>()) {
      const size_t CAP = sizeof(host.lxc_list) / sizeof(host.lxc_list[0]);
      for (JsonObjectConst c : p["lxcs"].as<JsonArrayConst>()) {
        if (host.lxc_list_count >= CAP) break;
        auto &dst = host.lxc_list[host.lxc_list_count++];
        dst.id = c["id"].isNull() ? -1 : c["id"].as<int32_t>();
        const char* nm = c["name"] | "";
        strncpy(dst.name, nm, sizeof(dst.name) - 1);
        const char* st = c["status"] | "";
        dst.running = (st && (strcmp(st, "running") == 0 || strcmp(st, "1") == 0));
      }
    }
  }

  // ---- IP block ----
  if (root["ip"].is<JsonObjectConst>()) {
    JsonObjectConst ip = root["ip"];
    safeCopy(host.primary_ifname, sizeof(host.primary_ifname), ip["primary_ifname"] | "");
    safeCopy(host.primary_ipv4,   sizeof(host.primary_ipv4),   ip["primary_ipv4"]   | "");
    safeCopy(host.gateway_ipv4,   sizeof(host.gateway_ipv4),   ip["gateway_ipv4"]   | "");
    safeCopy(host.ip_status,      sizeof(host.ip_status),      ip["ip_status"]      | "");
  } else {
    host.primary_ifname[0] = 0;
    host.primary_ipv4[0]   = 0;
    host.gateway_ipv4[0]   = 0;
    host.ip_status[0]      = 0;
  }

  // ---- NET load (totals preferred) ----
  host.net_rx_kbps  = NAN;
  host.net_tx_kbps  = NAN;
  host.net_window_s = 0;

  if (root["net"].is<JsonObjectConst>()) {
    JsonObjectConst n = root["net"];

    if (!n["window_s"].isNull()) host.net_window_s = n["window_s"].as<uint16_t>();

    bool gotTotals = false;
    if (!n["total_rx_bps"].isNull() || !n["total_tx_bps"].isNull()) {
      uint64_t rx_bps = n["total_rx_bps"].isNull() ? 0ULL : n["total_rx_bps"].as<uint64_t>();
      uint64_t tx_bps = n["total_tx_bps"].isNull() ? 0ULL : n["total_tx_bps"].as<uint64_t>();
      host.net_rx_kbps = (float)rx_bps / 1000.0f;
      host.net_tx_kbps = (float)tx_bps / 1000.0f;
      gotTotals = true;
    }
    if (!gotTotals && (!n["total_rx_Bps"].isNull() || !n["total_tx_Bps"].isNull())) {
      uint64_t rx_Bps = n["total_rx_Bps"].isNull() ? 0ULL : n["total_rx_Bps"].as<uint64_t>();
      uint64_t tx_Bps = n["total_tx_Bps"].isNull() ? 0ULL : n["total_tx_Bps"].as<uint64_t>();
      host.net_rx_kbps = (float)(rx_Bps * 8ULL) / 1000.0f;
      host.net_tx_kbps = (float)(tx_Bps * 8ULL) / 1000.0f;
      gotTotals = true;
    }
    if (!gotTotals && n["interfaces"].is<JsonArrayConst>()) {
      JsonArrayConst arr = n["interfaces"].as<JsonArrayConst>();
      const char* pri = host.primary_ifname;
      JsonObjectConst best = JsonObjectConst();
      for (JsonObjectConst it : arr) {
        const char* ifn = it["if"] | "";
        if (pri && pri[0] && strcmp(ifn, pri) == 0) { best = it; break; }
        if (!best) best = it;
      }
      if (best) {
        uint64_t rx_Bps = best["rx_Bps"].isNull() ? 0ULL : best["rx_Bps"].as<uint64_t>();
        uint64_t tx_Bps = best["tx_Bps"].isNull() ? 0ULL : best["tx_Bps"].as<uint64_t>();
        host.net_rx_kbps = (float)(rx_Bps * 8ULL) / 1000.0f;
        host.net_tx_kbps = (float)(tx_Bps * 8ULL) / 1000.0f;
      }
    }
  }

  // ---- DISKS ----
  host.disk_count = 0;
  if (root["disks"].is<JsonArrayConst>()) {
    JsonArrayConst arr = root["disks"].as<JsonArrayConst>();
    const size_t CAP = sizeof(host.disks) / sizeof(host.disks[0]);
    for (JsonObjectConst d : arr) {
      if (host.disk_count >= CAP) break;
      auto &dst = host.disks[host.disk_count++];

      const char* nm = d["name"] | "";
      safeCopy(dst.name, sizeof(dst.name), nm);

      const char* st = d["state"] | "";
      dst.active = stateIsActive(st);

      // Only trust/show temperature if disk is active.
      // For idle/standby we store a sentinel so the UI prints “-”
      if (dst.active) {
        int8_t tC = extractDiskTempC(d);
        if (d["temp_C"].isNull() && d["temp_c"].isNull() && d["temperature_C"].isNull()) {
          // Host didn’t send a temp even though active → keep sentinel
          dst.temp_c = -127;
        } else {
          dst.temp_c = tC;
        }
      } else {
        dst.temp_c = -127; // sentinel → UI prints "-"
      }
    }
  }

  return true;
}
