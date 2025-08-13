#pragma once
#include "display_manager.h"

class PageDisks : public IPage {
public:
  const char* title() const override { return "Disks"; }
  void render(Epd_t& d, const HostState& host, const UiState& ui) override;
};
