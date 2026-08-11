#include "LocalSettingsMenu.hpp"

#include <cstdio>

#define ASSERT_TRUE(expr)                                                                                  \
  do {                                                                                                     \
    if (!(expr)) {                                                                                         \
      printf("    ✗ %s:%d expected true: %s\n", __FILE__, __LINE__, #expr);                              \
      return 1;                                                                                            \
    }                                                                                                      \
  } while (0)

#define ASSERT_EQ(a, b)                                                                                    \
  do {                                                                                                     \
    auto _a = (a);                                                                                         \
    auto _b = (b);                                                                                         \
    if (_a != _b) {                                                                                        \
      printf("    ✗ %s:%d expected equality\n", __FILE__, __LINE__);                                     \
      return 1;                                                                                            \
    }                                                                                                      \
  } while (0)

using PoolController::LocalMenuItem;
using PoolController::LocalSettingsMenu;
using PoolController::LocalUiEvent;
using PoolController::LocalUiPage;

static int test_short_press_opens_menu() {
  LocalSettingsMenu menu;
  ASSERT_EQ(menu.currentPage(), LocalUiPage::OVERVIEW);
  menu.handleEvent(LocalUiEvent::SHORT_PRESS);
  ASSERT_EQ(menu.currentPage(), LocalUiPage::MENU);
  ASSERT_TRUE(menu.isMenuActive());
  ASSERT_EQ(menu.currentMenuItem(), LocalMenuItem::MODE);
  return 0;
}

static int test_rotate_pages_on_overview() {
  LocalSettingsMenu menu;
  menu.handleEvent(LocalUiEvent::ROTATE_CLOCKWISE);
  ASSERT_EQ(menu.currentPage(), LocalUiPage::NETWORK);
  menu.handleEvent(LocalUiEvent::ROTATE_CLOCKWISE);
  ASSERT_EQ(menu.currentPage(), LocalUiPage::SYSTEM);
  menu.handleEvent(LocalUiEvent::ROTATE_COUNTER_CLOCKWISE);
  ASSERT_EQ(menu.currentPage(), LocalUiPage::NETWORK);
  return 0;
}

static int test_menu_navigation_and_qr_selection() {
  LocalSettingsMenu menu;
  menu.handleEvent(LocalUiEvent::SHORT_PRESS);
  menu.handleEvent(LocalUiEvent::ROTATE_CLOCKWISE);
  menu.handleEvent(LocalUiEvent::ROTATE_CLOCKWISE);
  menu.handleEvent(LocalUiEvent::ROTATE_CLOCKWISE);
  ASSERT_EQ(menu.currentMenuItem(), LocalMenuItem::QR_CODE);
  menu.handleEvent(LocalUiEvent::SHORT_PRESS);
  ASSERT_EQ(menu.currentPage(), LocalUiPage::QRCODE);
  ASSERT_TRUE(!menu.isMenuActive());
  return 0;
}

static int test_long_press_returns_to_overview() {
  LocalSettingsMenu menu;
  menu.handleEvent(LocalUiEvent::ROTATE_CLOCKWISE);
  ASSERT_EQ(menu.currentPage(), LocalUiPage::NETWORK);
  menu.handleEvent(LocalUiEvent::LONG_PRESS);
  ASSERT_EQ(menu.currentPage(), LocalUiPage::OVERVIEW);
  return 0;
}

int run_local_settings_menu_tests() {
  int failures = 0;
  failures += test_short_press_opens_menu();
  failures += test_rotate_pages_on_overview();
  failures += test_menu_navigation_and_qr_selection();
  failures += test_long_press_returns_to_overview();
  if (failures == 0) {
    printf("  LocalSettingsMenu Tests: 4 passed, 0 failed\n");
  }
  return failures;
}
