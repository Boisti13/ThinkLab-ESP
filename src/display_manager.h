#pragma once
#include <GxEPD2_BW.h>
#include "state.h"
#include "config.h"

// Concrete display type
using Epd_t = GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>;

struct IPage {
  virtual const char* title() const = 0;
  virtual void render(Epd_t& d, const HostState& host, const UiState& ui) = 0;
  virtual ~IPage() = default;
};

class DisplayManager {
public:
  DisplayManager();
  void begin();                        // init display (silent), set rotation
  void toast(const __FlashStringHelper* msg);

  void registerPage(IPage* page, bool isDebug=false);
  uint8_t pageCount(const UiState& ui) const;
  void renderCurrent(const HostState& host, const UiState& ui);

  // Access to underlying display for splash etc.
  Epd_t& display() { return epd; }

private:
  Epd_t epd;
  static const uint8_t MAXP = 8;
  IPage* pages[MAXP] = {nullptr};
  bool   pagesIsDebug[MAXP] = {false};
  uint8_t total = 0;

  int mapUiIndexToReal(const UiState& ui) const;
};
