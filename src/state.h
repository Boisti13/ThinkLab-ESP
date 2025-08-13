#pragma once
#include <Arduino.h>
#include <cstdint>
#include <math.h>

// ------------ small fixed sizes for strings ------------
static constexpr size_t HOSTNAME_LEN = 32;
static constexpr size_t IFNAME_LEN = 16;
static constexpr size_t IPV4_LEN = 16; // "255.255.255.255"
static constexpr size_t STATUS_LEN = 20;

static constexpr uint8_t MAX_DISKS = 6;
static constexpr uint8_t MAX_GUESTS = 6;

// ------------ helpers used by pages ------------
inline float bytesToGiB(uint64_t b) { return (float)b / 1073741824.0f; } // binary GiB

inline float percent_u64(uint64_t part, uint64_t whole)
{
  if (whole == 0)
    return NAN;
  return (100.0f * (float)part) / (float)whole;
}

inline String fmtUptime(uint32_t s)
{
  uint32_t d = s / 86400U;
  uint32_t h = (s % 86400U) / 3600U;
  uint32_t m = (s % 3600U) / 60U;
  char buf[24];
  snprintf(buf, sizeof(buf), "%ud %uh %um", d, h, m);
  return String(buf);
}

inline uint32_t secsSince(uint32_t t_ms)
{
  return t_ms ? (millis() - t_ms) / 1000U : 0U;
}

// ------------ structs ------------
struct DiskInfo
{
  char name[16] = {0};
  int16_t temp_c = -127; // -127 = unknown
  bool active = false;   // recent I/O or "active"
};

struct GuestInfo
{
  int32_t id = -1;
  char name[18] = {0};  // 17 chars + NUL fits 200px nicely with our font
  bool running = false; // status == "running"
};

enum DisplayMode : uint8_t
{
  MODE_TOUCH = 0,
  MODE_AUTO = 1
};

struct HostState
{
  // top-level
  uint32_t uptime_sec = 0;
  char hostname[HOSTNAME_LEN] = {0};

  // CPU
  float cpu_percent = NAN;
  float load1 = NAN;  // from "cpu.load1"
  float load5 = NAN;  // from "cpu.load5"
  float load15 = NAN; // from "cpu.load15"
  // RAM
  uint64_t ram_total = 0;
  uint64_t ram_used = 0;

  // Filesystem (root "/")
  uint64_t fs_root_total = 0;
  uint64_t fs_root_used = 0;

  // Proxmox counts
  int32_t vms_running = -1;
  int32_t vms_total = -1;
  int32_t lxcs_running = -1;
  int32_t lxcs_total = -1;

  // Proxmox guest lists (names + status)
  uint8_t vm_list_count = 0;
  GuestInfo vm_list[MAX_GUESTS];

  uint8_t lxc_list_count = 0;
  GuestInfo lxc_list[MAX_GUESTS];

  // Disks
  uint8_t disk_count = 0;
  DiskInfo disks[MAX_DISKS];

  // IP block
  char primary_ifname[IFNAME_LEN] = {0};
  char primary_ipv4[IPV4_LEN] = {0};
  char gateway_ipv4[IPV4_LEN] = {0};
  char ip_status[STATUS_LEN] = {0};

  // Network load (totals, preferred; stored as kbps for UI)
  float net_rx_kbps = NAN;
  float net_tx_kbps = NAN;
  uint16_t net_window_s = 0;

  // Local sensors (ESP-side)
  float local_temp_c = NAN;

  // Debug/preview
  String last_json; // last received JSON (truncated by RX buffer if needed)
};

struct UiState
{
  // Mode & feature flags
  DisplayMode mode = MODE_AUTO;
  bool debugEnabled = true;

  // Page navigation state
  uint8_t currentPage = 0;      // used by main.ino / display_manager
  bool firstRenderDone = false; // splash → first full render

  // Serial/JSON stats
  uint32_t lastParseOkMs = 0; // millis()
  uint16_t lastJsonLen = 0;   // bytes
  uint32_t parseOkCount = 0;
  uint32_t parseErrCount = 0;
  uint32_t rxOverflowCnt = 0;

  bool firstDataReady = false;
};
