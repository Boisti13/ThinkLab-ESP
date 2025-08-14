#pragma once
#include "config.h"
#include "state.h"

#include <GxEPD2_BW.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// Panel/type alias: 200x200 SSD1681
using Epd_t = GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>;

// Simple page interface
struct IPage {
  virtual ~IPage() {}
  // Match existing pages (return const char*)
  virtual const char* title() const = 0;
  virtual void render(Epd_t& d, const HostState& host, const UiState& ui) = 0;
};

class DisplayManager {
public:
  DisplayManager();

  void   begin();                               // init display
  void   toast(const __FlashStringHelper* msg); // simple centered message

  void   registerPage(IPage* page, bool isDebug);
  uint8_t pageCount(const UiState& ui) const;   // counts pages included in rotation
  void   renderCurrent(const HostState& host, const UiState& ui);

  Epd_t& display();                              // access to underlying GxEPD2

private:
  struct Entry { IPage* page; bool isDebug; };
  static constexpr uint8_t MAX_PAGES = 12;

  Entry   _pages[MAX_PAGES];
  uint8_t _count;

  // Map UiState.currentPage (filtered index) to actual index in _pages[]
  int     mapUiIndexToReal(const UiState& ui) const;

  // E‑ink display instance (pins: CS, DC, RST, BUSY) — constructed with panel object
  Epd_t   _display;
};
