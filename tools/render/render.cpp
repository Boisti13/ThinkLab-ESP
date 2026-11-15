// Renders the e-paper pages with made-up data into PBM files (see README.md).
#include <cstdio>
#include <string>

#include "WiFi.h"
#include "config.h"
#include "pages/page_debug.h"
#include "pages/page_disks.h"
#include "pages/page_network.h"
#include "pages/page_overview.h"
#include "pages/page_vms.h"
#include "state.h"

FakeWiFi WiFi;
static uint32_t g_now = 3600000;  // the board has been up an hour
uint32_t millis() { return g_now; }

// Fan readings for the Debug page
uint8_t fan1PwmGetPercent() { return 25; }
int fan1TachGetRPM() { return 1180; }
int fan2TachGetRPM() { return -1; }

static void copyStr(char *dst, size_t n, const char *src) {
  strncpy(dst, src, n - 1);
  dst[n - 1] = 0;
}

static void guest(GuestInfo &g, int id, const char *name, bool running) {
  g.id = id;
  copyStr(g.name, sizeof g.name, name);
  g.running = running;
}

static void demo(HostState &h, UiState &ui) {
  h.uptime_sec = 9 * 86400 + 5 * 3600;
  copyStr(h.hostname, sizeof h.hostname, "pve");
  h.cpu_percent = 23.7f;
  h.load1 = 1.56f;
  h.load5 = 1.21f;
  h.load15 = 0.98f;
  h.ram_total = 16ULL << 30;
  h.ram_used = (uint64_t)(7.2 * (1ULL << 30));
  h.fs_root_total = 94ULL << 30;
  h.fs_root_used = 21ULL << 30;

  h.vms_running = 1;
  h.vms_total = 2;
  h.lxcs_running = 7;
  h.lxcs_total = 9;
  guest(h.vm_list[0], 100, "homeassistant", true);
  guest(h.vm_list[1], 120, "windows-test", false);
  h.vm_list_count = 2;
  const char *cts[] = {"dns", "proxy", "media", "books", "vpn", "backup"};
  for (int i = 0; i < 6; i++) guest(h.lxc_list[i], 101 + i, cts[i], i != 2);
  h.lxc_list_count = 6;

  const char *disks[] = {"nvme0n1", "sda", "sdb", "sdc"};
  for (int i = 0; i < 4; i++) {
    copyStr(h.disks[i].name, sizeof h.disks[i].name, disks[i]);
    h.disks[i].active = (i < 2);
  }
  h.disk_count = 4;

  copyStr(h.primary_ifname, sizeof h.primary_ifname, "vmbr0");
  copyStr(h.primary_ipv4, sizeof h.primary_ipv4, "192.168.1.10/24");
  copyStr(h.gateway_ipv4, sizeof h.gateway_ipv4, "192.168.1.1");
  copyStr(h.ip_status, sizeof h.ip_status, "ok");
  h.net_rx_kbps = 1364.7f;
  h.net_tx_kbps = 977.5f;
  h.net_window_s = 120;
  h.local_temp_c = 38.2f;
  h.fan_duty_cmd = 25.0f;
  h.fan_duty_filt = 25.0f;
  h.fan_active = 1;

  ui.firstDataReady = true;
  ui.lastParseOkMs = g_now - 42000;
  ui.lastJsonLen = 3110;
  ui.parseOkCount = 30;
  ui.parseErrCount = 0;

  WiFi.ip.o[0] = 192; WiFi.ip.o[1] = 168; WiFi.ip.o[2] = 1; WiFi.ip.o[3] = 50;
}

static void writePbm(Epd_t &d, const std::string &path) {
  FILE *f = fopen(path.c_str(), "w");
  fprintf(f, "P1\n%d %d\n", d.width(), d.height());
  for (int y = 0; y < d.height(); y++) {
    for (int x = 0; x < d.width(); x++) fputs(d.getPixel(x, y) ? "0 " : "1 ", f);  // set bit = white
    fputc('\n', f);
  }
  fclose(f);
}

int main(int argc, char **argv) {
  std::string out = argc > 1 ? argv[1] : ".";
  static HostState host;
  static UiState ui;
  demo(host, ui);

  PageOverview overview;
  PageDisks disks;
  PageVMs vms;
  PageNetwork network;
  PageDebug debug;
  struct { IPage *p; const char *name; } pages[] = {
      {&overview, "overview"}, {&disks, "disks"}, {&vms, "vms"}, {&network, "network"}, {&debug, "debug"}};
  for (auto &pg : pages) {
    static Epd_t d;
    d.fillScreen(GxEPD_WHITE);
    pg.p->render(d, host, ui);
    writePbm(d, out + "/eink-" + pg.name + ".pbm");
  }
  return 0;
}
