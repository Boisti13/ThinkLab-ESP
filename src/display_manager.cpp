#include "display_manager.h"

// ---------- ctor ----------
// IMPORTANT: Construct GxEPD2_BW with a *panel* object, not raw pins.
DisplayManager::DisplayManager()
: _count(0),
  _display(GxEPD2_154_D67(/*CS=*/7, /*DC=*/1, /*RST=*/2, /*BUSY=*/3))
{}

// ---------- begin ----------
void DisplayManager::begin() {
  // Matches your known‑good init
  _display.init(115200, true, 50, false);
  _display.setRotation(3);
}

// ---------- toast (simple center text) ----------
void DisplayManager::toast(const __FlashStringHelper* msg) {
  Epd_t& d = _display;
  d.firstPage();
  do {
    d.fillScreen(GxEPD_WHITE);
    d.setTextColor(GxEPD_BLACK);
    d.setFont(&FreeMonoBold9pt7b);

    int16_t x1, y1; uint16_t w, h;
    d.getTextBounds((const __FlashStringHelper*)msg, 0, 0, &x1, &y1, &w, &h);
    int16_t x = (d.width()  - (int16_t)w) / 2;
    int16_t y = (d.height() + (int16_t)h) / 2;
    d.setCursor(x, y);
    d.print(msg);
  } while (d.nextPage());
}

// ---------- display getter ----------
Epd_t& DisplayManager::display() { return _display; }

// ---------- register pages ----------
void DisplayManager::registerPage(IPage* p, bool isDebug) {
  if (_count >= MAX_PAGES) return;
  _pages[_count++] = Entry{p, isDebug};
}

// ---------- count pages in rotation ----------
uint8_t DisplayManager::pageCount(const UiState& ui) const {
  uint8_t n = 0;
  const bool includeDbg = (ui.debugEnabled && DEBUG_IN_ROTATION);
  for (uint8_t i = 0; i < _count; ++i) {
    if (_pages[i].isDebug) {
      if (includeDbg) ++n;     // include Debug only when allowed
    } else {
      ++n;
    }
  }
  return n;
}

// ---------- map filtered index -> real index ----------
int DisplayManager::mapUiIndexToReal(const UiState& ui) const {
  const bool includeDbg = (ui.debugEnabled && DEBUG_IN_ROTATION);
  uint8_t want = ui.currentPage;
  for (uint8_t i = 0; i < _count; ++i) {
    if (_pages[i].isDebug && !includeDbg) continue;
    if (want == 0) return i;
    --want;
  }
  return -1;
}

// ---------- render current page ----------
void DisplayManager::renderCurrent(const HostState& host, const UiState& ui) {
  uint8_t avail = pageCount(ui);
  if (avail == 0) return;

  int real = mapUiIndexToReal(ui);
  if (real < 0) {
    // Fallback: render the first allowed page
    const bool includeDbg = (ui.debugEnabled && DEBUG_IN_ROTATION);
    for (uint8_t i = 0; i < _count; ++i) {
      if (_pages[i].isDebug && !includeDbg) continue;
      _pages[i].page->render(_display, host, ui);
      return;
    }
    return;
  }

  _pages[real].page->render(_display, host, ui);
}
