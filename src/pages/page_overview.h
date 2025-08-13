#pragma once
#include "display_manager.h"

class PageOverview : public IPage {
public:
  const char* title() const override { return "Overview"; }
  void render(Epd_t& d, const HostState& host, const UiState& ui) override;
};
