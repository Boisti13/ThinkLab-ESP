// Stand-in for GxEPD2_BW (tools/render): draws into a 1-bit canvas instead of the panel.
#pragma once
#include <Adafruit_GFX.h>
#define GxEPD_BLACK 0x0000
#define GxEPD_WHITE 0xFFFF
struct GxEPD2_154_D67 {
  static const uint16_t WIDTH = 200;
  static const uint16_t HEIGHT = 200;
};
template <typename Panel, uint16_t page_height> class GxEPD2_BW : public GFXcanvas1 {
public:
  template <typename... A> GxEPD2_BW(A...) : GFXcanvas1(Panel::WIDTH, Panel::HEIGHT) {}
  void init(...) {}
  void setRotation(uint8_t r) { GFXcanvas1::setRotation(r); }
  void setFullWindow() {}
  void setPartialWindow(int16_t, int16_t, int16_t, int16_t) {}
  void firstPage() {}
  bool nextPage() { return false; }  // one pass draws the whole screen
  void display(bool = false) {}
  void hibernate() {}
  void powerOff() {}
};
