#include "display_manager.h"
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

DisplayManager::DisplayManager()
  : epd(GxEPD2_154_D67(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY)) {}

void DisplayManager::begin() {
  epd.init();           // silent (no serial chatter)
  epd.setRotation(3);
}

void DisplayManager::toast(const __FlashStringHelper* msg) {
  epd.firstPage();
  do {
    epd.fillScreen(GxEPD_WHITE);
    epd.setTextColor(GxEPD_BLACK);
    epd.setFont(&FreeMonoBold9pt7b);
    epd.setCursor(10, 40);
    epd.print(msg);
  } while (epd.nextPage());
}

void DisplayManager::registerPage(IPage* p, bool isDebug) {
  if (!p || total >= MAXP) return;
  pages[total] = p;
  pagesIsDebug[total] = isDebug;
  total++;
}

uint8_t DisplayManager::pageCount(const UiState& ui) const {
  uint8_t c = 0;
  for (uint8_t i = 0; i < total; ++i) {
    if (pages[i] == nullptr) continue;
    if (pagesIsDebug[i] && !ui.debugEnabled) continue;
    c++;
  }
  return c;
}

int DisplayManager::mapUiIndexToReal(const UiState& ui) const {
  // Convert ui.currentPage (0..N-1 of enabled pages) to actual slot index
  uint8_t want = ui.currentPage;
  for (uint8_t i = 0; i < total; ++i) {
    if (pages[i] == nullptr) continue;
    if (pagesIsDebug[i] && !ui.debugEnabled) continue;
    if (want == 0) return i;
    want--;
  }
  // Fallback to first enabled
  for (uint8_t i = 0; i < total; ++i) {
    if (pages[i] && !(pagesIsDebug[i] && !ui.debugEnabled)) return i;
  }
  return -1;
}

void DisplayManager::renderCurrent(const HostState& host, const UiState& ui) {
  int idx = mapUiIndexToReal(ui);
  if (idx < 0) return;
  pages[idx]->render(epd, host, ui);
}
