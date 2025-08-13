#pragma once
#include "display_manager.h"

class PageDebug : public IPage {
public:
  const char* title() const override { return "Debug"; }
  void render(Epd_t& d, const HostState& host, const UiState& ui) override;
};
