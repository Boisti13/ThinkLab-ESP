#pragma once
#include "display_manager.h"

class PageVMs : public IPage {
public:
  const char* title() const override { return "VMs/LXCs"; }
  void render(Epd_t& d, const HostState& host, const UiState& ui) override;
};
