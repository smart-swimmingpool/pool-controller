// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

#include "LocalSettingsMenu.hpp"

namespace PoolController {
namespace {

constexpr std::uint8_t kPageCount{4};
constexpr std::uint8_t kMenuItemCount{5};

}  // namespace

void LocalSettingsMenu::handleEvent(LocalUiEvent event) {
  if (event == LocalUiEvent::NONE) {
    return;
  }
  if (event == LocalUiEvent::LONG_PRESS) {
    returnToOverview();
    return;
  }
  if (currentPage_ == LocalUiPage::MENU) {
    if (event == LocalUiEvent::ROTATE_CLOCKWISE) {
      nextMenuItem();
    } else if (event == LocalUiEvent::ROTATE_COUNTER_CLOCKWISE) {
      previousMenuItem();
    } else if (event == LocalUiEvent::SHORT_PRESS) {
      selectMenuItem();
    }
    return;
  }
  if (event == LocalUiEvent::SHORT_PRESS) {
    currentPage_ = LocalUiPage::MENU;
    currentMenuItem_ = LocalMenuItem::MODE;
    needsRedraw_ = true;
  } else if (event == LocalUiEvent::ROTATE_CLOCKWISE) {
    nextPage();
  } else if (event == LocalUiEvent::ROTATE_COUNTER_CLOCKWISE) {
    previousPage();
  }
}

void LocalSettingsMenu::nextPage() {
  const auto page = static_cast<std::uint8_t>(currentPage_);
  const auto next = static_cast<std::uint8_t>((page + 1U) % kPageCount);
  currentPage_ = static_cast<LocalUiPage>(next);
  needsRedraw_ = true;
}

void LocalSettingsMenu::previousPage() {
  const auto page = static_cast<std::uint8_t>(currentPage_);
  const auto prev = static_cast<std::uint8_t>((page + kPageCount - 1U) % kPageCount);
  currentPage_ = static_cast<LocalUiPage>(prev);
  needsRedraw_ = true;
}

void LocalSettingsMenu::nextMenuItem() {
  const auto item = static_cast<std::uint8_t>(currentMenuItem_);
  const auto next = static_cast<std::uint8_t>((item + 1U) % kMenuItemCount);
  currentMenuItem_ = static_cast<LocalMenuItem>(next);
  needsRedraw_ = true;
}

void LocalSettingsMenu::previousMenuItem() {
  const auto item = static_cast<std::uint8_t>(currentMenuItem_);
  const auto prev = static_cast<std::uint8_t>((item + kMenuItemCount - 1U) % kMenuItemCount);
  currentMenuItem_ = static_cast<LocalMenuItem>(prev);
  needsRedraw_ = true;
}

void LocalSettingsMenu::selectMenuItem() {
  switch (currentMenuItem_) {
    case LocalMenuItem::NETWORK_STATUS:
      currentPage_ = LocalUiPage::NETWORK;
      break;
    case LocalMenuItem::QR_CODE:
      currentPage_ = LocalUiPage::QRCODE;
      break;
    case LocalMenuItem::EXIT:
    case LocalMenuItem::MODE:
    case LocalMenuItem::PUMP:
      currentPage_ = LocalUiPage::OVERVIEW;
      break;
  }
  needsRedraw_ = true;
}

void LocalSettingsMenu::returnToOverview() {
  currentPage_ = LocalUiPage::OVERVIEW;
  needsRedraw_ = true;
}

}  // namespace PoolController
