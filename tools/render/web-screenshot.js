// Screenshot of the board's web page with made-up values (see README.md).
//   THINKLAB_WEB_USER=... THINKLAB_WEB_PASS=... node web-screenshot.js http://<board> docs/screenshots/web.png
// Fetches the live page once, replaces every real value (addresses, SSID,
// uptimes, temperatures, disks) with demo data, answers the page's API calls
// with demo JSON and renders it offline. Nothing is sent to the board but the
// one GET of the page.
const { chromium } = require('playwright');

const [, , base, out = 'docs/screenshots/web.png'] = process.argv;
if (!base) { console.error('usage: node web-screenshot.js http://<board> [out.png]'); process.exit(2); }
const auth = 'Basic ' + Buffer.from(`${process.env.THINKLAB_WEB_USER}:${process.env.THINKLAB_WEB_PASS}`).toString('base64');

const tag = (v) => `<div class=tag>${v}</div>`;
function demo(html) {
  const row = (label, value) => {
    const re = new RegExp(`(<div class=row><div>${label}</div>)<div class=tag>[^<]*(<)`, 'g');
    html = html.replace(re, `$1<div class=tag>${value}$2`);
  };
  // ESP32 card, then the Proxmox card (same labels, so in page order)
  let n = 0;
  html = html.replace(/(<div class=row><div>IP<\/div>)<div class=tag>[^<]*</g,
    (_, a) => `${a}<div class=tag>${n++ === 0 ? '192.168.1.50' : '192.168.1.10'}<`);
  n = 0;
  html = html.replace(/(<div class=row><div>Uptime<\/div>)<div class=tag>[^<]*</g,
    (_, a) => `${a}<div class=tag>${n++ === 0 ? '3d 4h 12m' : '9d 5h 31m'}<`);
  row('Hostname', 'ThinkLab-esp32');
  row('SSID', 'HomeWiFi');
  row('Case Temp', '38.2&deg;C');
  html = html.replace(/(<div class=tag>)-?\d+ dBm/, '$1-52 dBm');
  // Disks: four demo disks, two active
  const disks = [['nvme0n1', true], ['sda', true], ['sdb', false], ['sdc', false]];
  html = html.replace(/(<div class=row><div>- [^<]*<\/div><div><span class='disk[^']*' title='[^']*'><\/span><\/div><\/div>)+/,
    disks.map(([d, on]) => `<div class=row><div>- ${d}</div><div><span class='disk ${on ? 'on' : ''}' title='${on ? 'active' : 'idle'}'></span></div></div>`).join(''));
  row('Disks', String(disks.length));
  html = html.replace(/<script>setTimeout\(\(\)=>location\.reload\(\),\d+\)<\/script>/, '');
  return html;
}

(async () => {
  const res = await fetch(base + '/', { headers: { Authorization: auth } });
  if (!res.ok) throw new Error(`GET / -> ${res.status}`);
  const html = demo(await res.text());

  // CHROMIUM_PATH: use an already installed browser instead of Playwright's own
  const browser = await chromium.launch(process.env.CHROMIUM_PATH ? { executablePath: process.env.CHROMIUM_PATH } : {});
  const page = await browser.newPage({ viewport: { width: 760, height: 900 }, deviceScaleFactor: 2 });
  const fake = 'http://thinklab-esp32.local';
  await page.route('**/*', (route) => {
    const u = new URL(route.request().url());
    const json = (o) => route.fulfill({ contentType: 'application/json', body: JSON.stringify(o) });
    if (u.pathname === '/') return route.fulfill({ contentType: 'text/html; charset=utf-8', body: html });
    if (u.pathname === '/api/ui') return json({ mode: 'TOUCH', in_debug: false, page: 0 });
    if (u.pathname === '/api/fan/status') return json({ auto: true, pct: 25, rpm: 1180, out_pct: 25 });
    return route.fulfill({ status: 404, body: '' });
  });
  await page.goto(fake + '/');
  await page.waitForTimeout(300);
  await page.screenshot({ path: out, fullPage: true });
  await browser.close();
  console.log(out);
})().catch((e) => { console.error(e.message); process.exit(1); });
