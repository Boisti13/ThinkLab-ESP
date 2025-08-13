#pragma once
#include "display_manager.h"

class PageNetwork : public IPage {
public:
  const char* title() const override { return "Network"; }
  void render(Epd_t& d, const HostState& host, const UiState& ui) override;
};
