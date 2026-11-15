#include "config.h"
#if USE_WIFI

#include "web_server.h"
#include "state.h"           // HostState
#include "display_manager.h" // DisplayManager, renderCurrent()
#include "ui_theme.h"        // ui::dataIsStale()
#include "modules/wifi_ota.h" // wifiOtaImageState(), upload/reboot hooks
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <uri/UriBraces.h>

#if USE_FAN1
#include "modules/fan1_pwm.h"
#include "modules/fan1_tach.h"
#include "modules/fan2_tach.h"
#endif
#if USE_FANCTRL
#include "modules/fanctrl.h"
#endif

#ifndef WEB_TITLE
#define WEB_TITLE "ThinkLab Dash"
#endif

// Access to UI/display/host globals from main.cpp
extern UiState g_ui;
extern DisplayManager g_disp;
extern HostState g_host;

// ====== Globals ======
WebServer server(80);

// ====== Favicon (kept minimal) ======
static const char FAVICON_SVG[] PROGMEM = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
  <text x="50%" y="50%" dy="0.35em" text-anchor="middle"
        font-family="Segoe UI, Roboto, Arial, sans-serif"
        font-size="42" font-weight="bold">
    <tspan fill="#d00">T</tspan><tspan fill="#000">L</tspan>
  </text>
</svg>
)SVG";

// ---- helpers ----
static int rssiToBars(int rssi)
{
    if (rssi >= -55)
        return 4;
    if (rssi >= -65)
        return 3;
    if (rssi >= -75)
        return 2;
    if (rssi >= -85)
        return 1;
    return 0;
}

static bool checkAuth()
{
    if (!server.authenticate(WEB_USER, WEB_PASS))
    {
        server.requestAuthentication();
        return false;
    }
    return true;
}

// --- UI control endpoints (JSON) ---

static void handleUiStatus()
{
    if (!checkAuth())
        return;
    String out;
    out.reserve(128);
    out += "{";
    out += "\"mode\":\"";
    out += (g_ui.mode == MODE_AUTO ? "AUTO" : "TOUCH");
    out += "\",";
    out += "\"in_debug\":";
    out += (g_ui.inDebugMode ? "true" : "false");
    out += ",";
    out += "\"page\":";
    out += String(g_ui.currentPage);
    out += "}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", out);
}

// Single-tap equivalent: re-render current page and arm advance window
static void handleUiPageUpdate()
{
    if (!checkAuth())
        return;

    if (!g_ui.inDebugMode)
    {
        g_disp.renderCurrent(g_host, g_ui);                       // refresh current page now
        g_ui.advanceArmUntilMs = millis() + TOUCH_ADVANCE_ARM_MS; // arm window
    }
    else
    {
        g_ui.lastDebugRefresh = 0; // nudge debug to refresh soon
    }
    server.send(204);
}

// Next page: advance index & render (no double-tap)
static void handleUiPageNext()
{
    if (!checkAuth())
        return;

    if (!g_ui.inDebugMode)
    {
        uint8_t n = g_disp.pageCount(g_ui);
        if (n > 0)
        {
            g_ui.currentPage = (g_ui.currentPage + 1) % n;
        }
        g_disp.renderCurrent(g_host, g_ui);
        g_ui.advanceArmUntilMs = 0; // explicit next cancels the arm window
    }
    else
    {
        g_ui.lastDebugRefresh = 0;
    }
    server.send(204);
}

// Toggle AUTO/TOUCH mode
static void handleUiToggleMode()
{
    if (!checkAuth())
        return;

    if (g_ui.mode == MODE_AUTO)
    {
        g_ui.mode = MODE_TOUCH;
    }
    else
    {
        g_ui.mode = MODE_AUTO;
    }
    server.send(204);
}

// Debug show/hide (forces debug page on/off)
static void handleUiShowDebug()
{
    if (!checkAuth())
        return;
    g_ui.inDebugMode = true;
    g_ui.lastDebugRefresh = 0;
    server.send(204);
}
static void handleUiHideDebug()
{
    if (!checkAuth())
        return;
    g_ui.inDebugMode = false;
    g_ui.lastDebugRefresh = 0;
    g_ui.tapPending = false;
    g_ui.advanceArmUntilMs = 0;
    server.send(204);
    g_disp.renderCurrent(g_host, g_ui);   // back to the normal page at once (after the reply)
}

// --- Fan control endpoints (use real controller enable/disable) ---
static void handleFanStatus()
{
    if (!checkAuth())
        return;

    bool autoMode = true;
#if USE_FANCTRL
    autoMode = fanCtrlGetEnabled();
#endif

    int pct = 0;        // current applied PWM %
    int rpm = -1;       // tach RPM, -1 = invalid
    float outPct = NAN; // controller "FanOut" (%), from fanctrl/HostState

#if USE_FAN1
    pct = (int)fan1PwmGetPercent();
    rpm = fan1TachGetRPM();
#endif

    // FanOut comes from HostState, same as on Debug page
    outPct = g_host.fan_duty_filt;

    String out;
    out.reserve(96);
    out += "{";
    out += "\"auto\":";
    out += (autoMode ? "true" : "false");
    out += ",";
    out += "\"pct\":";
    out += String(pct);
    out += ",";
    out += "\"rpm\":";
    out += String(rpm);
    out += ",";
    out += "\"out_pct\":";
    if (isnan(outPct))
    {
        out += "null";
    }
    else
    {
        out += String(outPct, 1); // one decimal place
    }
    int rpm2 = -1;
#if USE_FAN2_TACH
    rpm2 = fan2TachGetRPM();
#endif
    out += ",\"rpm2\":" + String(rpm2);
#if USE_FANCTRL
    out += ",\"stalled\":" + String(fanCtrlStalled() ? "true" : "false");
#endif
    out += ",\"manual_left_s\":";
#if USE_FANCTRL
    out += String(fanCtrlManualLeftS());
#else
    out += "0";
#endif
    out += "}";

    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", out);
}

// POST /api/fan/manual/<0..100>: manual duty (ends after FAN_MANUAL_MAX_MS or at
// FAN_MANUAL_OVERRIDE_C, see fanctrl). The web page's buttons use 25 and 50.
static void handleFanManual()
{
    if (!checkAuth())
        return;
    const String arg = server.pathArg(0);
    char *end = nullptr;
    const long pct = strtol(arg.c_str(), &end, 10);
    if (arg.length() == 0 || *end != '\0' || pct < 0 || pct > 100)
    {
        server.send(400, "text/plain", "duty must be 0..100");
        return;
    }
#if USE_FANCTRL
    fanCtrlSetManual((uint8_t)pct);
#elif USE_FAN1
    fan1PwmSetPercent((uint8_t)pct);
#endif
    server.send(204);
}

static void handleFanAuto()
{
    if (!checkAuth())
        return;
#if USE_FANCTRL
    fanCtrlSetEnabled(true);
#endif
    server.send(204);
}

// --- ESP control endpoint (reboot ESP32 client) ---
static void handleEspReboot()
{
    if (!checkAuth())
        return;
    wifiOta_RequestReboot();
    server.send(200, "text/plain", "Reboot requested");
}

// ================= HTML PAGES =================
static void handleRoot()
{
    if (!checkAuth())
        return;

    const bool linkUp = WiFi.isConnected();
    const IPAddress ip = WiFi.localIP();
    const long rssi = linkUp ? WiFi.RSSI() : -127;
    const int bars = rssiToBars((int)rssi);

    String html;
    html.reserve(9000);
    html += "<!doctype html>"
            "<meta charset='utf-8'>"
            "<meta name=viewport content='width=device-width,initial-scale=1'>"
            "<title>" +
            String(WEB_TITLE) + "</title>"
                                "<link rel='icon' type='image/svg+xml' href='/favicon.svg'>"
                                "<style>"
                                "body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;"
                                "max-width:720px;margin:24px auto;padding:0 12px}"
                                "h2{margin:0 0 12px}"
                                ".card{border:1px solid #ddd;border-radius:12px;padding:16px;margin:12px 0}"
                                ".row{display:flex;justify-content:space-between;margin:6px 0}"
                                ".tag{font:12px/1.6 monospace;background:#f4f4f4;padding:2px 6px;border-radius:6px}"
                                ".bars{display:inline-block;vertical-align:middle;margin-left:8px}"
                                ".bar{display:inline-block;width:6px;margin-right:2px;background:#bbb;border-radius:2px}"
                                ".bar.on{background:#222}"
                                ".disk{display:inline-block;width:10px;height:10px;border:1px solid #333;"
                                "border-radius:2px;vertical-align:middle}"
                                ".disk.on{background:#222}";

    html += R"RAW(
/* Button styles */
.btn{
  -webkit-tap-highlight-color:transparent;
  appearance:none;
  border:1px solid #ccc;
  background:#111;
  color:#fff;
  padding:.6rem .9rem;
  border-radius:10px;
  font-weight:600;
  cursor:pointer;
  transition:background .2s ease, transform .05s ease, box-shadow .15s ease;
}
.btn:hover{ background:#000; transform:translateY(-1px); box-shadow:0 3px 6px rgba(0,0,0,.2); }
.btn:active{ transform:translateY(0); box-shadow:0 1px 2px rgba(0,0,0,.1); }

/* Ghost variant (outline) */
.btn-ghost{
  background:transparent;
  color:inherit;
  border:1px solid #ccc;
}
.btn-ghost:hover{ background:rgba(0,0,0,.05); }

/* Danger variant */
.btn-danger{
  background:#b00020;
  border-color:#b00020;
  color:#fff;
}
.btn-danger:hover{
  background:#a0001c;
}

)RAW";

    html += "</style>";

    html += "<h2>ThinkLab Web Viewer</h2>";

    // --- ESP32 card ---
    html += "<div class=card><b>ESP32-C3</b>";
    html += "<div class=row><div>Hostname</div><div class=tag>" + String(HOSTNAME) + "</div></div>";
    html += "<div class=row><div>IP</div><div class=tag>" + (linkUp ? ip.toString() : "-") + "</div></div>";
    html += "<div class=row><div>RSSI</div><div class=tag>";
    if (linkUp)
    {
        html += String(rssi) + " dBm<span class=bars>";
        for (int i = 0; i < 4; i++)
        {
            html += String("<span class='bar ") + (i < bars ? "on" : "") +
                    "' style='height:" + String((i + 1) * 6) + "px'></span>";
        }
        html += "</span>";
    }
    else
    {
        html += "disconnected";
    }
    html += "</div></div>";
    html += "<div class=row><div>SSID</div><div class=tag>" + (linkUp ? WiFi.SSID() : "-") + "</div></div>";

    // Case temperature (Dallas)
    {
        const float tC = g_host.local_temp_c;
        html += "<div class=row><div>Case Temp</div><div class=tag>";
        if (isnan(tC))
        {
            html += "—";
        }
        else
        {
            html += String(tC, 1) + "&deg;C";
        }
        html += "</div></div>";
    }

    // ESP uptime
    {
        uint32_t up_s = millis() / 1000UL;
        uint32_t days = up_s / 86400UL;
        uint32_t hours = (up_s % 86400UL) / 3600UL;
        uint32_t mins = (up_s % 3600UL) / 60UL;
        html += "<div class=row><div>Uptime</div><div class=tag>" +
                String(days) + "d " + String(hours) + "h " + String(mins) + "m</div></div>";
    }

    html += "<div class=row><div>Build</div><div class=tag>" + String(BUILD_VERSION) + "</div></div>";

    // --- E-Ink controls (buttons) ---
    html += "<hr style='border:none;border-top:1px solid #eee;margin:12px 0'>";

    // Header row
    html += "<div class='row' style='font-weight:600'><div>E-Ink Display</div></div>";

    // Mode tag row (separate!)
    html += "<div class='row' style='margin-top:4px'>";
    html += "  <div class='tag' id='modeBadge'>mode: …</div>";
    html += "</div>";

    // Buttons row
    html += "<div style='display:flex;gap:8px;flex-wrap:wrap;margin-top:6px'>";
    html += "<button id='btnToggleMode'  class='btn btn-ghost'>Toggle Auto/Touch</button>";
    html += "<button id='btnToggleDebug' class='btn btn-ghost'>Show Debug Page</button>";
    html += "<button id='btnUpdatePage'  class='btn btn-ghost'>Update Page</button>";
    html += "<button id='btnNextPage'    class='btn btn-ghost'>Next Page</button>";
    html += "</div>";

    // --- Fan controls (right under E-Ink controls) ---
    html += "<div style='height:8px'></div>";
    html += "<div class='row' style='font-weight:600'><div>Cooling</div></div>";
    html += "<div style='display:flex;gap:8px;align-items:center;flex-wrap:wrap'>";
    html += "<button id='btnFan25'  class='btn btn-ghost'>Fan 25%</button>";
    html += "<button id='btnFan50'  class='btn btn-ghost'>Fan 50%</button>";
    html += "<button id='btnFanAuto' class='btn btn-ghost'>Fan Auto</button>";
    html += "<span id='fanBadge' class='tag'>Fan: …</span>";
    html += "</div>";

    // --- ESP control (reboot) ---
    html += "<div style='display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-top:50px'>";
    html += "<button id='btnRebootEsp' class='btn btn-danger'>Reboot ESP</button>";
    html += "<span class='tag'>Reboots ESP32 client only</span>";
    html += "</div>";
    html += "</div>"; // end ESP32 card

    // --- Proxmox card (minimal) ---
    html += "<div class=card><b>Proxmox</b>";
    if (g_host.primary_ipv4[0])
    {
        String proxStr(g_host.primary_ipv4);
        int slashPos = proxStr.indexOf('/');
        if (slashPos > 0)
            proxStr = proxStr.substring(0, slashPos); // strip CIDR
        html += "<div class=row><div>IP</div><div class=tag>" + proxStr + "</div></div>";
    }
    else
    {
        html += "<div class=row><div>IP</div><div class=tag>-</div></div>";
    }

    if (g_host.uptime_sec > 0)
    {
        uint32_t up_s = g_host.uptime_sec;
        uint32_t days = up_s / 86400U;
        uint32_t hours = (up_s % 86400U) / 3600U;
        uint32_t mins = (up_s % 3600U) / 60U;
        html += "<div class=row><div>Uptime</div><div class=tag>" +
                String(days) + "d " + String(hours) + "h " + String(mins) + "m</div></div>";
    }
    else
    {
        html += "<div class=row><div>Uptime</div><div class=tag>-</div></div>";
    }

    if (g_host.disk_count == 0)
    {
        html += "<div class=row><div>Disks</div><div class=tag>-</div></div>";
    }
    else
    {
        html += "<div class=row><div>Disks</div><div class=tag>" + String(g_host.disk_count) + "</div></div>";
        const uint8_t n = g_host.disk_count;
        for (uint8_t i = 0; i < n; ++i)
        {
            const DiskInfo &dk = g_host.disks[i];
            const String name = dk.name[0] ? String(dk.name) : String("-");
            html += "<div class=row><div>- " + name + "</div><div>"
                                                      "<span class='disk " +
                    String(dk.active ? "on" : "") + "' "
                                                    "title='" +
                    String(dk.active ? "active" : "idle") + "'></span>"
                                                            "</div></div>";
        }
        html += "<div class=row><div></div><div>"
                "<span class='disk on' style='margin-right:6px'></span>"
                "<span class='tag' style='margin-right:12px'>active</span>"
                "<span class='disk' style='margin-right:6px'></span>"
                "<span class='tag'>idle</span>"
                "</div></div>";
    }
    html += "</div>"; // end Proxmox card

    // --- Actions ---
    html += "<div class=card><b>Actions</b>";
    html += "<div class=row><div>Firmware</div><div><a href='/update'>Upload</a></div></div>";
    html += "<div class=row><div>API</div><div><a href='/status.json'>status.json</a></div></div>";
    html += "</div>";

    // --- Status JSON script helper ---
    html += R"JS(
<script>
function uiApplyModeVisibility(j){
  const show = (j.mode === 'TOUCH') && !j.in_debug;
  const upd  = document.getElementById('btnUpdatePage');
  const nxt  = document.getElementById('btnNextPage');
  if (upd) upd.style.display = show ? 'inline-block' : 'none';
  if (nxt) nxt.style.display = show ? 'inline-block' : 'none';
}

async function uiFetchStatus(){
  const r = await fetch('/api/ui', {cache:'no-store'});
  const j = await r.json();
  const badge = document.getElementById('modeBadge');
  const btnDbg = document.getElementById('btnToggleDebug');
  badge.textContent = 'mode: ' + j.mode + (j.in_debug ? ' (DEBUG)' : '');
  btnDbg.textContent = j.in_debug ? 'Exit Debug Page' : 'Show Debug Page';
  uiApplyModeVisibility(j);
}

document.getElementById('btnToggleMode').addEventListener('click', async ()=>{
  await fetch('/api/ui/mode/toggle', {method:'POST'});
  uiFetchStatus();
});
document.getElementById('btnToggleDebug').addEventListener('click', async ()=>{
  const on = document.getElementById('btnToggleDebug').textContent.includes('Exit');
  await fetch(on ? '/api/ui/debug/hide' : '/api/ui/debug/show', {method:'POST'});
  uiFetchStatus();
});
document.getElementById('btnUpdatePage').addEventListener('click', async ()=>{
  await fetch('/api/ui/page/update', {method:'POST'});
  uiFetchStatus();
});
document.getElementById('btnNextPage').addEventListener('click', async ()=>{
  await fetch('/api/ui/page/next', {method:'POST'});
  uiFetchStatus();
});

// Fan buttons + status badge
async function fanFetchStatus(){
  const r = await fetch('/api/fan/status', {cache:'no-store'});
  if (!r.ok) return;
  const j = await r.json();
  const badge = document.getElementById('fanBadge');
  if (badge){
    const modeStr = j.auto ? 'AUTO' : ('MANUAL ' + j.pct + '%' + (j.manual_left_s ? ' (' + Math.ceil(j.manual_left_s / 60) + ' min left)' : ''));

    const r = (v) => (typeof v === 'number' && v >= 0) ? v : '—';
    let rpmStr = r(j.rpm) + ' / ' + r(j.rpm2) + ' RPM';
    if (j.stalled) rpmStr += ' (STALLED)';

    let outStr = '';
    if (typeof j.out_pct === 'number'){
      // Display as integer percentage
      outStr = 'Out ' + j.out_pct.toFixed(0) + '%';
    }

    // fan: AUTO | 820 RPM | Out 37%
    badge.textContent = 'fan: ' + modeStr + ' | ' + rpmStr + (outStr ? (' | ' + outStr) : '');
  }
}
document.getElementById('btnFan25').addEventListener('click', async ()=>{
  await fetch('/api/fan/manual/25', {method:'POST'});
  fanFetchStatus();
});
document.getElementById('btnFan50').addEventListener('click', async ()=>{
  await fetch('/api/fan/manual/50', {method:'POST'});
  fanFetchStatus();
});
document.getElementById('btnFanAuto').addEventListener('click', async ()=>{
  await fetch('/api/fan/auto', {method:'POST'});
  fanFetchStatus();
});

// ESP reboot button
const btnReboot = document.getElementById('btnRebootEsp');
if (btnReboot){
  btnReboot.addEventListener('click', async ()=>{
    if (!confirm('Really reboot the ESP client?')) return;
    btnReboot.disabled = true;
    const oldText = btnReboot.textContent;
    btnReboot.textContent = 'Rebooting…';
    try{
      await fetch('/api/esp/reboot', {method:'POST'});
    }catch(e){
      btnReboot.disabled = false;
      btnReboot.textContent = oldText;
      alert('Error sending reboot: ' + e);
    }
  });
}

uiFetchStatus();
fanFetchStatus();
</script>
)JS";

    html += "<script>setTimeout(()=>location.reload(),5000)</script>";
    server.send(200, "text/html; charset=utf-8", html);
}

// --- OTA page + upload ---
static void handleUpdatePage()
{
    if (!checkAuth())
        return;
    server.send(200, "text/html; charset=utf-8",
                R"HTML(<!doctype html>
<meta charset="utf-8">
<title>OTA Update</title>
<body style="font-family: system-ui, -apple-system, Segoe UI, Roboto, Arial, sans-serif; max-width: 720px; margin: 40px auto;">
<h2>OTA Firmware Update</h2>
<form method="POST" action="/update" enctype="multipart/form-data">
  <input type="file" name="update">
  <button type="submit" class="btn">Upload</button>
</form>
</body>)HTML");
}

static void handleUpdateUpload()
{
    if (!checkAuth())
        return;

    HTTPUpload &up = server.upload();
    static bool updateBegun = false;

    if (up.status == UPLOAD_FILE_START)
    {
        updateBegun = false;
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        {
            server.send(500, "text/plain", "Update.begin() failed");
            return;
        }
        wifiOta_SetInUpload(true);
    }
    else if (up.status == UPLOAD_FILE_WRITE)
    {
        if (Update.write(up.buf, up.currentSize) != up.currentSize)
        {
            server.send(500, "text/plain", "Update.write() failed");
            wifiOta_SetInUpload(false);
            return;
        }
        updateBegun = true;
    }
    else if (up.status == UPLOAD_FILE_END)
    {
        wifiOta_SetInUpload(false);
        if (updateBegun && Update.end(true))
        {
            server.send(200, "text/plain", "OK, rebooting…");
            delay(250);
            ESP.restart();
            return;
        }
        else
        {
            server.send(500, "text/plain", "Update.end() failed");
            return;
        }
    }
}

// --- JSON status (ESP + subset) ---
static void handleStatusJson()
{
    if (!checkAuth())
        return;
    const bool linkUp = WiFi.isConnected();
    String out;
    out.reserve(512);
    out += "{";
    out += "\"hostname\":\"" + String(HOSTNAME) + "\",";
    out += "\"ip\":\"" + (linkUp ? WiFi.localIP().toString() : String("")) + "\",";
    out += "\"ssid\":\"" + String(linkUp ? WiFi.SSID() : "") + "\",";
    out += "\"rssi_dbm\":" + String(linkUp ? WiFi.RSSI() : -127) + ",";
    out += "\"esp_uptime_sec\":" + String(millis() / 1000UL) + ",";
    out += "\"case_c\":";
    out += (isnan(g_host.local_temp_c) ? String("null") : String(g_host.local_temp_c, 1));
    out += ",";
    out += "\"build\":\"" + String(BUILD_VERSION) + "\",";
    out += "\"mac\":\"" + WiFi.macAddress() + "\",";
    // Host link: seconds since the last good answer, and whether the screen marks it stale
    out += "\"host_age_s\":";
    out += (g_ui.lastParseOkMs ? String((millis() - g_ui.lastParseOkMs) / 1000UL) : String("null"));
    out += ",\"host_stale\":";
    out += (ui::dataIsStale(g_ui) ? "true" : "false");
    out += ",\"parse_err\":" + String(g_ui.parseErrCount);
    out += ",\"image\":\"" + String(wifiOtaImageState()) + "\"";
    // Fan
    bool fanAuto = true;
    uint32_t manualLeft = 0;
#if USE_FANCTRL
    fanAuto = fanCtrlGetEnabled();
    manualLeft = fanCtrlManualLeftS();
#endif
    int fanPct = -1, fanRpm = -1;
#if USE_FAN1
    fanPct = (int)fan1PwmGetPercent();
    fanRpm = fan1TachGetRPM();
#endif
    out += ",\"fan_auto\":" + String(fanAuto ? "true" : "false");
    out += ",\"fan_pct\":" + String(fanPct);
    out += ",\"fan_rpm\":" + String(fanRpm);
    out += ",\"fan_manual_left_s\":" + String(manualLeft);
    out += ",\"fan_curve_pct\":" + (isnan(g_host.fan_duty_cmd) ? String("null") : String(g_host.fan_duty_cmd, 1));
    out += ",\"fan_on\":" + String(g_host.fan_active ? "true" : "false");
    int fan2Rpm = -1;
#if USE_FAN2_TACH
    fan2Rpm = fan2TachGetRPM();
#endif
    out += ",\"fan2_rpm\":" + String(fan2Rpm);
#if USE_FANCTRL
    out += ",\"fan_min_pct\":" + String(fanCtrlMinDuty());
    out += ",\"fan_stalled\":" + String(fanCtrlStalled() ? "true" : "false");
    out += ",\"fan_stall_kicks\":" + String(fanCtrlStallKicks());
#endif
    out += "}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", out);
}

// ================= Public API =================
void webServerSetup()
{
    server.on("/", HTTP_GET, handleRoot);
    server.on("/status.json", HTTP_GET, handleStatusJson);

    // UI control endpoints
    server.on("/api/ui", HTTP_GET, handleUiStatus);
    server.on("/api/ui/mode/toggle", HTTP_POST, handleUiToggleMode);
    server.on("/api/ui/debug/show", HTTP_POST, handleUiShowDebug);
    server.on("/api/ui/debug/hide", HTTP_POST, handleUiHideDebug);
    server.on("/api/ui/page/update", HTTP_POST, handleUiPageUpdate);
    server.on("/api/ui/page/next", HTTP_POST, handleUiPageNext);

    // Fan control endpoints
    server.on("/api/fan/status", HTTP_GET, handleFanStatus);
    server.on(UriBraces("/api/fan/manual/{}"), HTTP_POST, handleFanManual);
    server.on("/api/fan/auto", HTTP_POST, handleFanAuto);

    // ESP control endpoints
    server.on("/api/esp/reboot", HTTP_POST, handleEspReboot);

    // OTA
    server.on("/update", HTTP_GET, handleUpdatePage);
    server.on("/update", HTTP_POST, []() {}, handleUpdateUpload);

    // Favicon
    server.on("/favicon.svg", HTTP_GET, []()
              { server.send(200, "image/svg+xml", FAVICON_SVG); });

    server.onNotFound([]()
                      {
        if (!checkAuth()) return;
        server.send(404, "text/plain", "Not found"); });

    server.begin();
}

void webServerLoop()
{
    server.handleClient();
}

#endif // USE_WIFI
