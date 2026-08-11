// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

#pragma once

#include "LocalUiTypes.hpp"

namespace PoolController {

class LocalSettingsMenu {
public:
  LocalSettingsMenu() = default;

  void handleEvent(LocalUiEvent event);

  LocalUiPage currentPage() const { return currentPage_; }
  LocalMenuItem currentMenuItem() const { return currentMenuItem_; }
  bool isMenuActive() const { return currentPage_ == LocalUiPage::MENU; }
  bool needsRedraw() const { return needsRedraw_; }
  void clearRedraw() { needsRedraw_ = false; }

private:
  void nextPage();
  void previousPage();
  void nextMenuItem();
  void previousMenuItem();
  void selectMenuItem();
  void returnToOverview();

  LocalUiPage currentPage_{LocalUiPage::OVERVIEW};
  LocalMenuItem currentMenuItem_{LocalMenuItem::MODE};
  bool needsRedraw_{true};
};

}  // namespace PoolController
