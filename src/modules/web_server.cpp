#include "config.h"
#if USE_WIFI

#include "web_server.h"
#include "state.h"              // HostState / DiskInfo
#include "display_manager.h"    // DisplayManager, renderCurrent()
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

#ifndef WEB_TITLE
#define WEB_TITLE "ThinkLab Dash"
#endif

// Access to UI/display/host globals from main.cpp
extern UiState        g_ui;
extern DisplayManager g_disp;
extern HostState      g_host;

// ====== Globals ======
WebServer server(80);

// ====== Favicon (transparent, T red, L black, +25% size) ======
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
static int rssiToBars(int rssi){
    if (rssi >= -55) return 4;
    if (rssi >= -65) return 3;
    if (rssi >= -75) return 2;
    if (rssi >= -85) return 1;
    return 0;
}
static bool checkAuth(){
    if (!server.authenticate(WEB_USER, WEB_PASS)){
        server.requestAuthentication();
        return false;
    }
    return true;
}

// --- UI control endpoints (JSON) ---

static void handleUiStatus(){
    if (!checkAuth()) return;
    String out;
    out.reserve(128);
    out += "{";
    out += "\"mode\":\""; out += (g_ui.mode == MODE_AUTO ? "AUTO" : "TOUCH"); out += "\",";
    out += "\"in_debug\":"; out += (g_ui.inDebugMode ? "true" : "false"); out += ",";
    out += "\"page\":"; out += String(g_ui.currentPage);
    out += "}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", out);
}

// Single-tap equivalent: re-render current page and arm advance window
static void handleUiPageUpdate(){
    if (!checkAuth()) return;

    if (!g_ui.inDebugMode){
        g_disp.renderCurrent(g_host, g_ui); // refresh current page now
        g_ui.advanceArmUntilMs = millis() + TOUCH_ADVANCE_ARM_MS; // arm double-press window
    } else {
        // In debug, just nudge the debug refresh to happen soon
        g_ui.lastDebugRefresh = 0;
    }
    server.send(204);
}

// Next page: advance index & render (no double-tap)
static void handleUiPageNext(){
    if (!checkAuth()) return;

    if (!g_ui.inDebugMode){
        uint8_t n = g_disp.pageCount(g_ui);
        if (n > 0){
            g_ui.currentPage = (g_ui.currentPage + 1) % n;
        }
        g_disp.renderCurrent(g_host, g_ui);
        g_ui.advanceArmUntilMs = 0; // explicit next cancels the arm window
    }
    server.send(204);
}

static void handleUiToggleMode(){
    if (!checkAuth()) return;
    g_ui.mode = (g_ui.mode == MODE_TOUCH) ? MODE_AUTO : MODE_TOUCH;
    handleUiStatus();
}

static void handleUiShowDebug(){
    if (!checkAuth()) return;
    g_ui.inDebugMode = true;
    g_ui.lastDebugRefresh = 0; // loop will draw debug promptly
    handleUiStatus();
}

static void handleUiHideDebug(){
    if (!checkAuth()) return;

    // Leave debug and immediately redraw normal page
    g_ui.inDebugMode       = false;
    g_ui.tapPending        = false;
    g_ui.advanceArmUntilMs = 0;
    g_disp.renderCurrent(g_host, g_ui);

    handleUiStatus();
}

// ================= HTML PAGES =================
static void handleRoot(){
    if (!checkAuth()) return;

    const bool       linkUp = WiFi.isConnected();
    const IPAddress  ip     = WiFi.localIP();
    const long       rssi   = linkUp ? WiFi.RSSI() : -127;
    const int        bars   = rssiToBars((int)rssi);

    String html;
    html.reserve(6000);
    html += "<!doctype html>"
            "<meta name=viewport content='width=device-width,initial-scale=1'>"
            "<title>" + String(WEB_TITLE) + "</title>"
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
            /* disk overview like EPD page */
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

/* Ghost variant (outline) — requested for all buttons */
.btn-ghost{
  background:transparent;
  color:inherit;
  border:1px solid #ccc;
}
.btn-ghost:hover{ background:rgba(0,0,0,.05); }
)RAW";

    html += "</style>";

    html += "<h2>ThinkLab Web Viewer</h2>";

    // --- ESP32 card ---
    html += "<div class=card><b>ESP32-C3</b>";
    html += "<div class=row><div>Hostname</div><div class=tag>" + String(HOSTNAME) + "</div></div>";
    html += "<div class=row><div>IP</div><div class=tag>" + (linkUp ? ip.toString() : "-") + "</div></div>";
    html += "<div class=row><div>RSSI</div><div class=tag>";
    if (linkUp){
        html += String(rssi) + " dBm<span class=bars>";
        for (int i = 0; i < 4; i++){
            html += String("<span class='bar ") + (i < bars ? "on" : "") +
                    "' style='height:" + String((i + 1) * 6) + "px'></span>";
        }
        html += "</span>";
    }else{
        html += "disconnected";
    }
    html += "</div></div>";
    html += "<div class=row><div>SSID</div><div class=tag>" + (linkUp ? WiFi.SSID() : "-") + "</div></div>";

    // ESP uptime (Xd Yh Zm)
    {
        uint32_t up_s  = millis() / 1000UL;
        uint32_t days  = up_s / 86400UL;
        uint32_t hours = (up_s % 86400UL) / 3600UL;
        uint32_t mins  = (up_s % 3600UL) / 60UL;
        html += "<div class=row><div>Uptime</div><div class=tag>" +
                String(days) + "d " + String(hours) + "h " + String(mins) + "m</div></div>";
    }

    html += "<div class=row><div>Build</div><div class=tag>" + String(BUILD_VERSION) + "</div></div>";

    // --- E-Ink controls (buttons) ---
    html += "<hr style='border:none;border-top:1px solid #eee;margin:10px 0'>";
    html += "<div style='display:flex;gap:8px;align-items:center;flex-wrap:wrap'>";
    html += "<button id='btnToggleMode'  class='btn btn-ghost'>Toggle Auto/Touch</button>";
    html += "<button id='btnToggleDebug' class='btn btn-ghost'>Show Debug Page</button>";
    html += "<button id='btnUpdatePage'  class='btn btn-ghost'>Update Page</button>";
    html += "<button id='btnNextPage'    class='btn btn-ghost'>Next Page</button>";
    html += "<span id='modeBadge' class='tag'>mode: …</span>";
    html += "</div>";

    html += R"JS(
<script>
function uiApplyModeVisibility(j){
  // Show Update/Next only in TOUCH and when not in Debug
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

uiFetchStatus();
</script>
)JS";

    html += "</div>"; // end ESP32 card

    // --- Proxmox card (IP:8006, uptime, disks overview) ---
    html += "<div class=card><b>Proxmox</b>";

    // IP:Port (clickable)
    if (g_host.primary_ipv4[0]){
        String proxStr(g_host.primary_ipv4);
        int slashPos = proxStr.indexOf('/');
        if (slashPos > 0) proxStr = proxStr.substring(0, slashPos); // strip CIDR
        html += "<div class=row><div>IP</div><div class=tag>"
                "<a href='https://" + proxStr + ":8006' target='_blank' rel='noopener'>"
                + proxStr + ":8006</a></div></div>";
    }else{
        html += "<div class=row><div>IP</div><div class=tag>-</div></div>";
    }

    // Host uptime (Xd Yh Zm)
    if (g_host.uptime_sec > 0){
        uint32_t up_s  = g_host.uptime_sec;
        uint32_t days  = up_s / 86400U;
        uint32_t hours = (up_s % 86400U) / 3600U;
        uint32_t mins  = (up_s % 3600U) / 60U;
        html += "<div class=row><div>Uptime</div><div class=tag>" +
                String(days) + "d " + String(hours) + "h " + String(mins) + "m</div></div>";
    }else{
        html += "<div class=row><div>Uptime</div><div class=tag>-</div></div>";
    }

    // Disks overview (active/idle)
    if (g_host.disk_count == 0){
        html += "<div class=row><div>Disks</div><div class=tag>-</div></div>";
    }else{
        html += "<div class=row><div>Disks</div><div class=tag>" + String(g_host.disk_count) + "</div></div>";
        const uint8_t n = g_host.disk_count;
        for (uint8_t i = 0; i < n; ++i){
            const DiskInfo &dk = g_host.disks[i];
            const String name = dk.name[0] ? String(dk.name) : String("-");
            html += "<div class=row><div>- " + name + "</div><div>"
                    "<span class='disk " + String(dk.active ? "on" : "") + "' "
                    "title='" + String(dk.active ? "active" : "idle") + "'></span>"
                    "</div></div>";
        }
        // Legend row
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

    html += "<script>setTimeout(()=>location.reload(),5000)</script>";
    server.send(200, "text/html", html);
}

static void handleStatusJson(){
    if (!checkAuth()) return;
    const bool linkUp = WiFi.isConnected();
    String out;
    out.reserve(256);
    out += "{";
    out += "\"hostname\":\"" + String(HOSTNAME) + "\",";
    out += "\"ip\":\"" + (linkUp ? WiFi.localIP().toString() : String("")) + "\",";
    out += "\"ssid\":\"" + String(linkUp ? WiFi.SSID() : "") + "\",";
    out += "\"rssi_dbm\":" + String(linkUp ? WiFi.RSSI() : -127) + ",";
    out += "\"esp_uptime_sec\":" + String(millis() / 1000UL) + ",";
    out += "\"build\":\"" + String(BUILD_VERSION) + "\"";
    out += "}";
    server.send(200, "application/json", out);
}

static void handleUpdatePage(){
    if (!checkAuth()) return;
    String html;
    html.reserve(800);
    html += "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
            "<h2>Upload .bin</h2>"
            "<form method='POST' action='/update' enctype='multipart/form-data'>"
            "<input type='file' name='firmware' accept='.bin' required>"
            "<button type='submit'>Update</button>"
            "</form>";
    server.send(200, "text/html", html);
}

static void handleUpdateUpload(){
    if (!checkAuth()) return;
    HTTPUpload &upload = server.upload();

    switch (upload.status){
    case UPLOAD_FILE_START:
        wifiOta_SetInUpload(true);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)){
            server.send(500, "text/plain", "Update begin failed");
            wifiOta_SetInUpload(false);
            return;
        }
        break;

    case UPLOAD_FILE_WRITE:
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize){
            Update.end();
            server.send(500, "text/plain", "Flash write failed");
            wifiOta_SetInUpload(false);
            return;
        }
        break;

    case UPLOAD_FILE_END:
        if (Update.end(true) && !Update.hasError()){
            server.sendHeader("Connection", "close");
            server.send(200, "text/html",
                        "<h3>Update successful.</h3><p>Rebooting… you can close this tab.</p>");
            wifiOta_RequestReboot();
        }else{
            server.send(500, "text/plain", "Update failed");
        }
        wifiOta_SetInUpload(false);
        break;

    case UPLOAD_FILE_ABORTED:
        Update.end();
        server.send(500, "text/plain", "Update aborted");
        wifiOta_SetInUpload(false);
        break;

    default:
        break;
    }
}

// ================= Public API =================
void webServerSetup(){
    server.on("/",           HTTP_GET,  handleRoot);
    server.on("/status.json",HTTP_GET,  handleStatusJson);

    // UI control endpoints
    server.on("/api/ui",               HTTP_GET,  handleUiStatus);
    server.on("/api/ui/mode/toggle",   HTTP_POST, handleUiToggleMode);
    server.on("/api/ui/debug/show",    HTTP_POST, handleUiShowDebug);
    server.on("/api/ui/debug/hide",    HTTP_POST, handleUiHideDebug);
    server.on("/api/ui/page/update",   HTTP_POST, handleUiPageUpdate);
    server.on("/api/ui/page/next",     HTTP_POST, handleUiPageNext);

    server.on("/update", HTTP_GET,  handleUpdatePage);
    server.on("/update", HTTP_POST, [](){}, handleUpdateUpload);

    server.on("/favicon.svg", HTTP_GET, [](){
        server.send(200, "image/svg+xml", FAVICON_SVG);
    });

    server.onNotFound([](){
        if (!checkAuth()) return;
        server.send(404, "text/plain", "Not found");
    });

    server.begin();
}

void webServerLoop(){
    server.handleClient();
}

#endif // USE_WIFI
