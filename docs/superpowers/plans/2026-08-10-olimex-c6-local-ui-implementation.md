# Olimex C6 Local UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an Olimex ESP32-C6-EVB hardware variant with a configurable 320x240 SPI TFT local display and KY-040 rotary encoder navigation.

**Architecture:** Keep pool-control logic unchanged and add a board-specific local UI path behind compile-time flags. Put the host-testable menu/event logic in small generic modules, then wrap Olimex-specific display and encoder hardware behind `OlimexLocalUi` so `PoolController.cpp` only wires `begin()` and `loop()`.

**Tech Stack:** Arduino ESP32 / PlatformIO, C++17, TFT_eSPI or Adafruit GFX display driver selected by implementation, existing `ConfigManager`/NVS settings, existing native CMake test runner.

## Global Constraints

- Board variant macro: `OLIMEX_ESP32_C6_EVB`.
- Local TFT feature macro: `HAS_LOCAL_TFT_UI`.
- Display: configurable 320x240 SPI TFT; 2.8" preferred, 2.4" acceptable.
- Driver configuration must support ST7789 or ILI9341-class controllers without hard-coding physical diagonal size.
- Display config constants: `DISPLAY_WIDTH = 320`, `DISPLAY_HEIGHT = 240`, `DISPLAY_DRIVER = ILI9341 or ST7789`, `DISPLAY_SIZE_CLASS = compact or normal`.
- Use 3.3 V logic only. ESP32-C6 GPIOs are not 5 V tolerant.
- TFT backlight must never be sourced directly from an ESP32 GPIO. GPIO may only control a transistor/MOSFET for PWM dimming.
- No full 320x240 RGB framebuffer requirement; use partial drawing/library-managed buffering.
- QR-code screen must use a dedicated page with black-on-white rendering and short URL payload.
- Existing `esp32dev` and `norvi_ae01_r` builds must keep compiling.
- No blocking UI loops; `PoolControllerContext::loop()` must keep reaching watchdog feeding.
- Do not persist menu cursor movement to NVS; only persist confirmed setting changes.
- Keep `graphify-out/` untracked and out of commits.

---

## File Structure

Create these focused modules:

- `src/LocalUiTypes.hpp` — shared UI enums/value objects (`LocalUiEvent`, `LocalUiPage`, `DisplayDriver`, `DisplaySizeClass`, `TftDisplayConfig`). No hardware includes.
- `src/LocalSettingsMenu.hpp` / `src/LocalSettingsMenu.cpp` — host-testable menu state machine. Consumes encoder-style UI events and produces page/menu state. No Arduino display code.
- `src/Ky040Decoder.hpp` / `src/Ky040Decoder.cpp` — host-testable KY-040 quadrature and button press decoding. No direct GPIO reads.
- `src/OlimexEncoderHandler.hpp` / `src/OlimexEncoderHandler.cpp` — Arduino GPIO wrapper for KY-040. Reads pins from `Config.hpp`, feeds `Ky040Decoder`, emits `LocalUiEvent` callbacks.
- `src/OlimexTftDisplay.hpp` / `src/OlimexTftDisplay.cpp` — TFT rendering wrapper for overview, menu, status, and QR pages. Compiled only under `OLIMEX_ESP32_C6_EVB` or `HAS_LOCAL_TFT_UI`.
- `src/OlimexLocalUi.hpp` / `src/OlimexLocalUi.cpp` — composes `OlimexEncoderHandler`, `LocalSettingsMenu`, and `OlimexTftDisplay`; exposes `begin()`, `loop()`, and `requestRedraw()`.

Modify these existing files:

- `src/Config.hpp` — add Olimex board pin mapping and display config constants.
- `platformio.ini` — add `olimex_esp32_c6_evb` environment and TFT/QR dependencies.
- `src/PoolController.cpp` — initialize and loop the Olimex local UI behind compile-time flags.
- `test/native/CMakeLists.txt` — add host-testable new `.cpp` files and test sources.
- `test/native/tests/test_main.cpp` — register new test suites.

Add these tests:

- `test/native/tests/test_local_settings_menu.cpp` — menu navigation, page switching, confirm/cancel behavior, QR page navigation.
- `test/native/tests/test_ky040_decoder.cpp` — clockwise/counter-clockwise quadrature, debounce, short press, long press.

---

### Task 1: Board Variant and Display Configuration

**Files:**

- Modify: `src/Config.hpp:75-151`
- Modify: `platformio.ini:29-88`

**Interfaces:**

- Produces: `OLIMEX_ESP32_C6_EVB`, `HAS_LOCAL_TFT_UI`, `PIN_TFT_*`, `PIN_ENCODER_*`, `TFT_DISPLAY_WIDTH`, `TFT_DISPLAY_HEIGHT`, `TFT_DISPLAY_SIZE_CLASS_COMPACT`, `TFT_DRIVER_ILI9341`, `TFT_DRIVER_ST7789` constants for later tasks.

- [ ] **Step 1: Add Olimex pin mapping in `src/Config.hpp`**

Insert an `#elif defined(OLIMEX_ESP32_C6_EVB)` block between the NORVI block and the default ESP32 block. Use UEXT SPI for the display and free GPIOs for encoder/button. Keep final pin choices easy to adjust after schematic/hardware verification.

```cpp
#elif defined(OLIMEX_ESP32_C6_EVB)

/** @brief DS18B20 data pin — solar collector temperature sensor. */
constexpr std::uint8_t PIN_DS_SOLAR{4};
/** @brief DS18B20 data pin — pool water temperature sensor. */
constexpr std::uint8_t PIN_DS_POOL{5};
/** @brief Relay control pin — pool circulation pump (Olimex relay). */
constexpr std::uint8_t PIN_RELAY_POOL{10};
/** @brief Relay control pin — solar heating pump (Olimex relay). */
constexpr std::uint8_t PIN_RELAY_SOLAR{11};
/** @brief Status LED — Olimex user LED. */
constexpr std::uint8_t PIN_LED_STATUS{8};
/** @brief Optional warning LED — not used on Olimex. */
constexpr std::int8_t PIN_LED_WARN{-1};

// ── Olimex local UI pins ────────────────────────────────────────────────────
constexpr std::uint8_t PIN_TFT_MOSI{18};
constexpr std::uint8_t PIN_TFT_SCLK{19};
constexpr std::uint8_t PIN_TFT_MISO{20};
constexpr std::uint8_t PIN_TFT_CS{21};
constexpr std::uint8_t PIN_TFT_DC{7};
constexpr std::uint8_t PIN_TFT_RST{6};
constexpr std::int8_t PIN_TFT_BACKLIGHT{-1};  // -1 = fixed 3.3 V backlight

constexpr std::uint8_t PIN_ENCODER_CLK{12};
constexpr std::uint8_t PIN_ENCODER_DT{13};
constexpr std::uint8_t PIN_ENCODER_SW{0};

constexpr std::uint16_t TFT_DISPLAY_WIDTH{320};
constexpr std::uint16_t TFT_DISPLAY_HEIGHT{240};
constexpr bool TFT_DISPLAY_SIZE_CLASS_COMPACT{false};
constexpr bool TFT_DRIVER_ILI9341{true};
constexpr bool TFT_DRIVER_ST7789{false};
```

Do not use GPIO1/2/3/15 for UI; those are Olimex opto inputs. Do not use GPIO10/11/22/23 except for relays.

- [ ] **Step 2: Add static assertions for display driver selection**

At the end of the namespace, before `}  // namespace PoolController`, add:

```cpp
#if defined(OLIMEX_ESP32_C6_EVB)
static_assert(TFT_DISPLAY_WIDTH == 320, "Olimex local UI expects a 320 px wide display");
static_assert(TFT_DISPLAY_HEIGHT == 240, "Olimex local UI expects a 240 px high display");
static_assert(TFT_DRIVER_ILI9341 != TFT_DRIVER_ST7789, "Select exactly one TFT driver");
static_assert(PIN_TFT_BACKLIGHT < 0 || PIN_TFT_BACKLIGHT != PIN_TFT_CS, "Backlight control pin must not conflict with TFT CS");
#endif
```

- [ ] **Step 3: Add PlatformIO environment**

Append this environment to `platformio.ini`:

```ini
[env:olimex_esp32_c6_evb]
platform = espressif32 @ 7.0.1
board = esp32-c6-devkitc-1
framework = arduino
build_flags =
  ; FW_VERSION is auto-maintained by release-please — do not edit manually
  '-D FW_VERSION="4.2.1"'  # x-release-please-version
  '-D GITHUB_REPO="smart-swimmingpool/pool-controller"'
  -D SERIAL_SPEED=${common.serial_speed}
  -D OLIMEX_ESP32_C6_EVB
  -D HAS_LOCAL_TFT_UI
  -D LOG_BUFFER_SIZE=8192
  -std=c++17
  -Wno-deprecated-declarations
build_unflags = -Werror=reorder
lib_deps =
  ${common_env_data.lib_deps}
  bodmer/TFT_eSPI @ ^2.5.43
  ricmoo/QRCode @ ^0.0.1
monitor_speed = ${common.serial_speed}
monitor_filters = esp32_exception_decoder, log2file, time, default
upload_speed = 115200
board_build.filesystem = littlefs
```

If PlatformIO rejects `esp32-c6-devkitc-1`, replace it with the closest board listed by `venv/bin/pio boards espressif32 | grep -i c6`, then document the replacement in the task commit message.

- [ ] **Step 4: Verify config syntax by building existing environments**

Run:

```bash
venv/bin/pio run -e esp32dev
venv/bin/pio run -e norvi_ae01_r
```

Expected: both builds pass. If either fails due unrelated existing dependency cache, run `venv/bin/pio pkg install` once and retry.

- [ ] **Step 5: Try the new Olimex environment**

Run:

```bash
venv/bin/pio run -e olimex_esp32_c6_evb
```

Expected: compile may fail because UI files do not exist yet. Acceptable failure for this task: missing `OlimexLocalUi`/display classes only. Not acceptable: unknown board, syntax errors, or missing shared libraries.

- [ ] **Step 6: Commit**

```bash
git add src/Config.hpp platformio.ini
git commit -m "build: add olimex esp32-c6 board variant"
```

---

### Task 2: Shared Local UI Types and Menu State Machine

**Files:**

- Create: `src/LocalUiTypes.hpp`
- Create: `src/LocalSettingsMenu.hpp`
- Create: `src/LocalSettingsMenu.cpp`
- Create: `test/native/tests/test_local_settings_menu.cpp`
- Modify: `test/native/CMakeLists.txt:69-88`
- Modify: `test/native/tests/test_main.cpp:101-126`

**Interfaces:**

- Produces: `enum class LocalUiEvent`, `enum class LocalUiPage`, `enum class LocalMenuItem`, `class LocalSettingsMenu`.
- Consumes: no hardware; later tasks call `LocalSettingsMenu::handleEvent(LocalUiEvent)`.

- [ ] **Step 1: Create `src/LocalUiTypes.hpp`**

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace PoolController {

enum class LocalUiEvent : std::uint8_t {
  NONE = 0,
  ROTATE_CLOCKWISE,
  ROTATE_COUNTER_CLOCKWISE,
  SHORT_PRESS,
  LONG_PRESS,
};

enum class LocalUiPage : std::uint8_t {
  OVERVIEW = 0,
  NETWORK,
  SYSTEM,
  QRCODE,
  MENU,
};

enum class LocalMenuItem : std::uint8_t {
  MODE = 0,
  PUMP,
  NETWORK_STATUS,
  QR_CODE,
  EXIT,
};

enum class DisplayDriver : std::uint8_t {
  ILI9341 = 0,
  ST7789,
};

enum class DisplaySizeClass : std::uint8_t {
  COMPACT = 0,
  NORMAL,
};

struct TftDisplayConfig {
  std::uint16_t width;
  std::uint16_t height;
  DisplayDriver driver;
  DisplaySizeClass sizeClass;
};

}  // namespace PoolController
```

- [ ] **Step 2: Write failing menu tests**

Create `test/native/tests/test_local_settings_menu.cpp`:

```cpp
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
```

- [ ] **Step 3: Register the failing test suite**

In `test/native/tests/test_main.cpp`, add:

```cpp
extern int run_local_settings_menu_tests();
```

and call it after `run_webportal_logs_tests()`:

```cpp
total += run_local_settings_menu_tests();
```

In `test/native/CMakeLists.txt`, add to `SERVICE_SOURCES`:

```cmake
  ${PROJ_ROOT}/src/LocalSettingsMenu.cpp
```

and add to `TEST_SOURCES`:

```cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_local_settings_menu.cpp
```

- [ ] **Step 4: Run test and verify it fails**

Run:

```bash
cmake --build test/native/build && test/native/build/test_runner
```

Expected: compilation fails because `LocalSettingsMenu.hpp` does not exist, or link fails because `LocalSettingsMenu` methods are not defined.

- [ ] **Step 5: Create `src/LocalSettingsMenu.hpp`**

```cpp
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
```

- [ ] **Step 6: Create `src/LocalSettingsMenu.cpp`**

```cpp
// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

#include "LocalSettingsMenu.hpp"

namespace PoolController {
namespace {

constexpr std::uint8_t kPageCount{4};      // OVERVIEW, NETWORK, SYSTEM, QRCODE
constexpr std::uint8_t kMenuItemCount{5};  // MODE, PUMP, NETWORK_STATUS, QR_CODE, EXIT

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
      currentPage_ = LocalUiPage::OVERVIEW;
      break;
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
```

- [ ] **Step 7: Run tests and verify pass**

Run:

```bash
cmake --build test/native/build && test/native/build/test_runner
```

Expected: all existing suites pass and `LocalSettingsMenu Tests: 4 passed, 0 failed` appears.

- [ ] **Step 8: Commit**

```bash
git add src/LocalUiTypes.hpp src/LocalSettingsMenu.hpp src/LocalSettingsMenu.cpp test/native/CMakeLists.txt test/native/tests/test_main.cpp test/native/tests/test_local_settings_menu.cpp
git commit -m "feat(ui): add local settings menu state machine"
```

---

### Task 3: KY-040 Decoder and Olimex Encoder Handler

**Files:**

- Create: `src/Ky040Decoder.hpp`
- Create: `src/Ky040Decoder.cpp`
- Create: `src/OlimexEncoderHandler.hpp`
- Create: `src/OlimexEncoderHandler.cpp`
- Create: `test/native/tests/test_ky040_decoder.cpp`
- Modify: `test/native/CMakeLists.txt`
- Modify: `test/native/tests/test_main.cpp`

**Interfaces:**

- Consumes: `LocalUiEvent` from Task 2, `PIN_ENCODER_*` from Task 1.
- Produces: `Ky040Decoder::update(bool clk, bool dt, bool swPressed, std::uint32_t nowMs) -> LocalUiEvent`; `OlimexEncoderHandler::begin()`, `loop()`, `onEvent()`.

- [ ] **Step 1: Write failing decoder tests**

Create `test/native/tests/test_ky040_decoder.cpp` with these cases:

```cpp
#include "Ky040Decoder.hpp"

#include <cstdio>

#define ASSERT_EQ(a, b)                                                                                    \
  do {                                                                                                     \
    auto _a = (a);                                                                                         \
    auto _b = (b);                                                                                         \
    if (_a != _b) {                                                                                        \
      printf("    ✗ %s:%d expected equality\n", __FILE__, __LINE__);                                     \
      return 1;                                                                                            \
    }                                                                                                      \
  } while (0)

using PoolController::Ky040Decoder;
using PoolController::LocalUiEvent;

static int test_clockwise_rotation() {
  Ky040Decoder decoder;
  ASSERT_EQ(decoder.update(true, true, false, 0), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(false, true, false, 5), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(false, false, false, 10), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, false, false, 15), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, false, 20), LocalUiEvent::ROTATE_CLOCKWISE);
  return 0;
}

static int test_counter_clockwise_rotation() {
  Ky040Decoder decoder;
  ASSERT_EQ(decoder.update(true, true, false, 0), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, false, false, 5), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(false, false, false, 10), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(false, true, false, 15), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, false, 20), LocalUiEvent::ROTATE_COUNTER_CLOCKWISE);
  return 0;
}

static int test_short_press_on_release() {
  Ky040Decoder decoder;
  ASSERT_EQ(decoder.update(true, true, true, 100), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, true, 300), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, false, 350), LocalUiEvent::SHORT_PRESS);
  return 0;
}

static int test_long_press_on_release() {
  Ky040Decoder decoder;
  ASSERT_EQ(decoder.update(true, true, true, 100), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, true, 2300), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, false, 2350), LocalUiEvent::LONG_PRESS);
  return 0;
}

int run_ky040_decoder_tests() {
  int failures = 0;
  failures += test_clockwise_rotation();
  failures += test_counter_clockwise_rotation();
  failures += test_short_press_on_release();
  failures += test_long_press_on_release();
  if (failures == 0) {
    printf("  KY-040 Decoder Tests: 4 passed, 0 failed\n");
  }
  return failures;
}
```

- [ ] **Step 2: Register and run failing test**

Add `test_ky040_decoder.cpp` to `TEST_SOURCES`, add `Ky040Decoder.cpp` to `SERVICE_SOURCES`, and register `run_ky040_decoder_tests()` in `test_main.cpp`. Run:

```bash
cmake --build test/native/build && test/native/build/test_runner
```

Expected: compile fails because `Ky040Decoder.hpp` does not exist.

- [ ] **Step 3: Create `src/Ky040Decoder.hpp`**

```cpp
#pragma once

#include "LocalUiTypes.hpp"

#include <cstdint>

namespace PoolController {

class Ky040Decoder {
public:
  LocalUiEvent update(bool clkHigh, bool dtHigh, bool swPressed, std::uint32_t nowMs);

private:
  static constexpr std::uint32_t LONG_PRESS_MS{2000};
  std::uint8_t lastState_{0x03};
  std::int8_t delta_{0};
  bool buttonWasPressed_{false};
  std::uint32_t buttonPressStartMs_{0};
};

}  // namespace PoolController
```

- [ ] **Step 4: Create `src/Ky040Decoder.cpp`**

```cpp
#include "Ky040Decoder.hpp"

namespace PoolController {

LocalUiEvent Ky040Decoder::update(bool clkHigh, bool dtHigh, bool swPressed, std::uint32_t nowMs) {
  const std::uint8_t state = static_cast<std::uint8_t>((clkHigh ? 0x02 : 0x00) | (dtHigh ? 0x01 : 0x00));
  const std::uint8_t transition = static_cast<std::uint8_t>((lastState_ << 2U) | state);
  lastState_ = state;

  switch (transition) {
    case 0b1110:
    case 0b1000:
    case 0b0001:
    case 0b0111:
      ++delta_;
      break;
    case 0b1101:
    case 0b0100:
    case 0b0010:
    case 0b1011:
      --delta_;
      break;
    default:
      break;
  }

  if (state == 0x03 && delta_ >= 4) {
    delta_ = 0;
    return LocalUiEvent::ROTATE_CLOCKWISE;
  }
  if (state == 0x03 && delta_ <= -4) {
    delta_ = 0;
    return LocalUiEvent::ROTATE_COUNTER_CLOCKWISE;
  }

  if (swPressed && !buttonWasPressed_) {
    buttonWasPressed_ = true;
    buttonPressStartMs_ = nowMs;
    return LocalUiEvent::NONE;
  }

  if (!swPressed && buttonWasPressed_) {
    buttonWasPressed_ = false;
    const std::uint32_t heldMs = nowMs - buttonPressStartMs_;
    return heldMs >= LONG_PRESS_MS ? LocalUiEvent::LONG_PRESS : LocalUiEvent::SHORT_PRESS;
  }

  return LocalUiEvent::NONE;
}

}  // namespace PoolController
```

- [ ] **Step 5: Create `src/OlimexEncoderHandler.hpp`**

```cpp
#pragma once

#include "LocalUiTypes.hpp"

#include <functional>

namespace PoolController {

class OlimexEncoderHandler {
public:
  using EventCallback = std::function<void(LocalUiEvent)>;

  static void begin();
  static void loop();
  static void onEvent(EventCallback cb) { eventCallback_ = cb; }

private:
  static EventCallback eventCallback_;
};

}  // namespace PoolController
```

- [ ] **Step 6: Create `src/OlimexEncoderHandler.cpp`**

```cpp
#include "OlimexEncoderHandler.hpp"

#if defined(OLIMEX_ESP32_C6_EVB)

#include "Config.hpp"
#include "Ky040Decoder.hpp"

#include <Arduino.h>

namespace PoolController {
namespace {

Ky040Decoder decoder;
constexpr std::uint32_t SAMPLE_INTERVAL_MS{2};
std::uint32_t lastSampleMs{0};

}  // namespace

OlimexEncoderHandler::EventCallback OlimexEncoderHandler::eventCallback_{};

void OlimexEncoderHandler::begin() {
  pinMode(PIN_ENCODER_CLK, INPUT_PULLUP);
  pinMode(PIN_ENCODER_DT, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
}

void OlimexEncoderHandler::loop() {
  const std::uint32_t nowMs = millis();
  if (nowMs - lastSampleMs < SAMPLE_INTERVAL_MS) {
    return;
  }
  lastSampleMs = nowMs;

  const bool clkHigh = digitalRead(PIN_ENCODER_CLK) == HIGH;
  const bool dtHigh = digitalRead(PIN_ENCODER_DT) == HIGH;
  const bool swPressed = digitalRead(PIN_ENCODER_SW) == LOW;
  const LocalUiEvent event = decoder.update(clkHigh, dtHigh, swPressed, nowMs);
  if (event != LocalUiEvent::NONE && eventCallback_) {
    eventCallback_(event);
  }
}

}  // namespace PoolController

#endif
```

- [ ] **Step 7: Run native tests**

Run:

```bash
cmake --build test/native/build && test/native/build/test_runner
```

Expected: all suites pass and `KY-040 Decoder Tests: 4 passed, 0 failed` appears.

- [ ] **Step 8: Commit**

```bash
git add src/Ky040Decoder.hpp src/Ky040Decoder.cpp src/OlimexEncoderHandler.hpp src/OlimexEncoderHandler.cpp test/native/CMakeLists.txt test/native/tests/test_main.cpp test/native/tests/test_ky040_decoder.cpp
git commit -m "feat(ui): add ky040 encoder input handling"
```

---

### Task 4: Olimex TFT Display Wrapper

**Files:**

- Create: `src/OlimexTftDisplay.hpp`
- Create: `src/OlimexTftDisplay.cpp`
- Modify: `platformio.ini` if TFT_eSPI needs build flags for pins/driver.

**Interfaces:**

- Consumes: `LocalUiPage`, `LocalMenuItem`, `TftDisplayConfig` from Task 2.
- Produces: `OlimexTftDisplay::begin()`, `drawOverview()`, `drawMenu()`, `drawNetwork()`, `drawSystem()`, `drawQrCode()`.

- [ ] **Step 1: Create `src/OlimexTftDisplay.hpp`**

```cpp
#pragma once

#include "LocalUiTypes.hpp"

namespace PoolController {

class OlimexTftDisplay {
public:
  static void begin();
  static void drawPage(LocalUiPage page, LocalMenuItem menuItem);
  static void requestRedraw() { forceRedraw_ = true; }

private:
  static void drawOverview();
  static void drawMenu(LocalMenuItem menuItem);
  static void drawNetwork();
  static void drawSystem();
  static void drawQrCode();
  static bool forceRedraw_;
};

}  // namespace PoolController
```

- [ ] **Step 2: Create `src/OlimexTftDisplay.cpp` with safe compile guards**

Use compile guards so non-Olimex builds do not pull TFT libraries:

```cpp
#include "OlimexTftDisplay.hpp"

#if defined(OLIMEX_ESP32_C6_EVB) && defined(HAS_LOCAL_TFT_UI)

#include "Config.hpp"
#include "ConfigManager.hpp"
#include "Version.h"

#include <Arduino.h>
#include <QRCode.h>
#include <TFT_eSPI.h>

namespace PoolController {
namespace {

TFT_eSPI tft;

void drawHeader(const char *left, const char *right) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(TFT_DISPLAY_SIZE_CLASS_COMPACT ? 1 : 2);
  tft.setCursor(8, 6);
  tft.print(left);
  tft.setCursor(240, 6);
  tft.print(right);
}

}  // namespace

bool OlimexTftDisplay::forceRedraw_{true};

void OlimexTftDisplay::begin() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  drawHeader("POOL", "BOOT");
  tft.setCursor(8, 40);
  tft.print("Starting...");
  forceRedraw_ = true;
}

void OlimexTftDisplay::drawPage(LocalUiPage page, LocalMenuItem menuItem) {
  if (!forceRedraw_) {
    return;
  }
  forceRedraw_ = false;
  tft.fillScreen(TFT_BLACK);
  switch (page) {
    case LocalUiPage::OVERVIEW:
      drawOverview();
      break;
    case LocalUiPage::NETWORK:
      drawNetwork();
      break;
    case LocalUiPage::SYSTEM:
      drawSystem();
      break;
    case LocalUiPage::QRCODE:
      drawQrCode();
      break;
    case LocalUiPage::MENU:
      drawMenu(menuItem);
      break;
  }
}

void OlimexTftDisplay::drawOverview() {
  drawHeader("POOL", ConfigManager::getSettings().opMode.c_str());
  tft.setTextSize(TFT_DISPLAY_SIZE_CLASS_COMPACT ? 3 : 4);
  tft.setCursor(8, 38);
  tft.print("--.- C");
  tft.setTextSize(2);
  tft.setCursor(8, 92);
  tft.print("Pumpe: --");
  tft.drawFastHLine(0, 120, TFT_DISPLAY_WIDTH, TFT_DARKGREY);
  drawHeader("SOLAR", "OK");
  tft.setTextSize(TFT_DISPLAY_SIZE_CLASS_COMPACT ? 3 : 4);
  tft.setCursor(8, 148);
  tft.print("--.- C");
  tft.setTextSize(2);
  tft.setCursor(8, 204);
  tft.print("Ventil: --");
}

void OlimexTftDisplay::drawMenu(LocalMenuItem menuItem) {
  drawHeader("MENU", "OK=Select");
  const char *items[] = {"Mode", "Pump", "Network", "QR Code", "Exit"};
  for (std::uint8_t i = 0; i < 5; ++i) {
    tft.setCursor(20, 42 + (i * 32));
    tft.setTextSize(2);
    tft.setTextColor(i == static_cast<std::uint8_t>(menuItem) ? TFT_BLACK : TFT_WHITE,
      i == static_cast<std::uint8_t>(menuItem) ? TFT_YELLOW : TFT_BLACK);
    tft.print(items[i]);
  }
}

void OlimexTftDisplay::drawNetwork() {
  drawHeader("NETWORK", "STATUS");
  tft.setTextSize(2);
  tft.setCursor(8, 48);
  tft.print("WiFi/MQTT status");
}

void OlimexTftDisplay::drawSystem() {
  drawHeader("SYSTEM", FW_VERSION);
  tft.setTextSize(2);
  tft.setCursor(8, 48);
  tft.print("Heap / uptime");
}

void OlimexTftDisplay::drawQrCode() {
  drawHeader("QR", "WEB UI");
  const char *url = "http://pool-controller.local";
  QRCode qrcode;
  std::uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, url);
  const std::uint8_t scale = 6;
  const std::uint16_t offsetX = 70;
  const std::uint16_t offsetY = 35;
  tft.fillRect(offsetX - 8, offsetY - 8, (qrcode.size * scale) + 16, (qrcode.size * scale) + 16, TFT_WHITE);
  for (std::uint8_t y = 0; y < qrcode.size; ++y) {
    for (std::uint8_t x = 0; x < qrcode.size; ++x) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(offsetX + (x * scale), offsetY + (y * scale), scale, scale, TFT_BLACK);
      }
    }
  }
}

}  // namespace PoolController

#endif
```

- [ ] **Step 3: Configure TFT_eSPI pins if needed**

If `TFT_eSPI` requires compile-time user setup, add build flags under `[env:olimex_esp32_c6_evb]` rather than editing library files:

```ini
  -D USER_SETUP_LOADED
  -D ILI9341_DRIVER
  -D TFT_WIDTH=240
  -D TFT_HEIGHT=320
  -D TFT_MOSI=18
  -D TFT_MISO=20
  -D TFT_SCLK=19
  -D TFT_CS=21
  -D TFT_DC=7
  -D TFT_RST=6
  -D LOAD_GLCD
  -D SPI_FREQUENCY=27000000
```

If the selected hardware is ST7789, replace `ILI9341_DRIVER` with the matching `ST7789_DRIVER` define documented by TFT_eSPI.

- [ ] **Step 4: Verify compile guards**

Run:

```bash
venv/bin/pio run -e esp32dev
venv/bin/pio run -e norvi_ae01_r
```

Expected: both builds pass without TFT library dependency leakage into non-Olimex environments.

- [ ] **Step 5: Build Olimex environment**

Run:

```bash
venv/bin/pio run -e olimex_esp32_c6_evb
```

Expected: build may still fail until Task 5 wires `OlimexLocalUi`; it must not fail due TFT_eSPI pin setup or QRCode dependency.

- [ ] **Step 6: Commit**

```bash
git add src/OlimexTftDisplay.hpp src/OlimexTftDisplay.cpp platformio.ini
git commit -m "feat(ui): add olimex tft display renderer"
```

---

### Task 5: Olimex Local UI Composition and PoolController Wiring

**Files:**

- Create: `src/OlimexLocalUi.hpp`
- Create: `src/OlimexLocalUi.cpp`
- Modify: `src/PoolController.cpp:267-516`

**Interfaces:**

- Consumes: `OlimexEncoderHandler`, `LocalSettingsMenu`, `OlimexTftDisplay`.
- Produces: `OlimexLocalUi::begin()`, `OlimexLocalUi::loop()`, `OlimexLocalUi::requestRedraw()`.

- [ ] **Step 1: Create `src/OlimexLocalUi.hpp`**

```cpp
#pragma once

namespace PoolController {

class OlimexLocalUi {
public:
  static void begin();
  static void loop();
  static void requestRedraw();
};

}  // namespace PoolController
```

- [ ] **Step 2: Create `src/OlimexLocalUi.cpp`**

```cpp
#include "OlimexLocalUi.hpp"

#if defined(OLIMEX_ESP32_C6_EVB) && defined(HAS_LOCAL_TFT_UI)

#include "LocalSettingsMenu.hpp"
#include "OlimexEncoderHandler.hpp"
#include "OlimexTftDisplay.hpp"

namespace PoolController {
namespace {

LocalSettingsMenu menu;

}  // namespace

void OlimexLocalUi::begin() {
  OlimexTftDisplay::begin();
  OlimexEncoderHandler::onEvent([](LocalUiEvent event) {
    menu.handleEvent(event);
    OlimexTftDisplay::requestRedraw();
  });
  OlimexEncoderHandler::begin();
  OlimexTftDisplay::drawPage(menu.currentPage(), menu.currentMenuItem());
}

void OlimexLocalUi::loop() {
  OlimexEncoderHandler::loop();
  if (menu.needsRedraw()) {
    OlimexTftDisplay::requestRedraw();
    menu.clearRedraw();
  }
  OlimexTftDisplay::drawPage(menu.currentPage(), menu.currentMenuItem());
}

void OlimexLocalUi::requestRedraw() {
  OlimexTftDisplay::requestRedraw();
}

}  // namespace PoolController

#else

namespace PoolController {

void OlimexLocalUi::begin() {}
void OlimexLocalUi::loop() {}
void OlimexLocalUi::requestRedraw() {}

}  // namespace PoolController

#endif
```

- [ ] **Step 3: Wire setup in `PoolController.cpp`**

Add include near existing NORVI includes:

```cpp
#if defined(OLIMEX_ESP32_C6_EVB) && defined(HAS_LOCAL_TFT_UI)
#include "OlimexLocalUi.hpp"
#endif
```

In `PoolControllerContext::setup()`, after `StatusLed::begin();`, add:

```cpp
#if defined(OLIMEX_ESP32_C6_EVB) && defined(HAS_LOCAL_TFT_UI)
  OlimexLocalUi::begin();
#endif
```

In `PoolControllerContext::loop()`, near the existing `NorviButtonHandler::loop()` / display loop location, add:

```cpp
#if defined(OLIMEX_ESP32_C6_EVB) && defined(HAS_LOCAL_TFT_UI)
  OlimexLocalUi::loop();
#endif
```

Keep the call non-blocking. Do not put display updates inside delays or `while` loops.

- [ ] **Step 4: Build all environments**

Run:

```bash
venv/bin/pio run -e esp32dev
venv/bin/pio run -e norvi_ae01_r
venv/bin/pio run -e olimex_esp32_c6_evb
```

Expected: all three builds pass. If `olimex_esp32_c6_evb` fails due specific board definition, fix the `board =` value in `platformio.ini` and re-run.

- [ ] **Step 5: Run native tests**

Run:

```bash
cmake --build test/native/build && test/native/build/test_runner
```

Expected: all suites pass.

- [ ] **Step 6: Commit**

```bash
git add src/OlimexLocalUi.hpp src/OlimexLocalUi.cpp src/PoolController.cpp
git commit -m "feat(ui): wire olimex local tft interface"
```

---

### Task 6: Safe First Hardware Smoke Checklist and Docs

**Files:**

- Create: `docs/olimex-esp32-c6-evb.md`
- Modify: `docs/superpowers/specs/2026-08-09-olimex-c6-local-ui-design.md` only if implementation discoveries change the concept.

**Interfaces:**

- Consumes: actual pin/config choices from Tasks 1-5.
- Produces: a safe bring-up checklist for hardware validation.

- [ ] **Step 1: Create hardware bring-up doc**

Create `docs/olimex-esp32-c6-evb.md` with:

```markdown
# Olimex ESP32-C6-EVB Variant

## Hardware

- Board: Olimex ESP32-C6-EVB
- Display: configurable 320x240 SPI TFT, 2.8" preferred, 2.4" acceptable
- Input: KY-040 rotary encoder with push button
- Logic level: 3.3 V only

## Safety Rules

- Do not feed 5 V into ESP32-C6 GPIOs.
- Do not drive the TFT backlight directly from an ESP32 GPIO.
- Power the TFT logic and KY-040 from 3.3 V.
- Verify display pinout, backlight polarity, and current limiting before wiring.

## Bring-Up Steps

1. Build `olimex_esp32_c6_evb`.
2. Power the board without TFT/encoder and verify serial boot.
3. Connect TFT VCC/GND/SPI/DC/RST with backlight fixed to 3.3 V if current-limited.
4. Verify the TFT boot screen.
5. Connect KY-040 CLK/DT/SW to the configured GPIOs.
6. Verify rotate clockwise/counter-clockwise changes pages.
7. Verify short press opens menu.
8. Verify long press returns to overview.
9. Open QR page and scan `http://pool-controller.local`.
10. Only after UI validation, connect real relay loads and opto inputs.
```

- [ ] **Step 2: Verify Markdown formatting**

Run:

```bash
npx prettier@3.5.3 --check "docs/olimex-esp32-c6-evb.md"
```

Expected: Prettier reports the doc is formatted.

- [ ] **Step 3: Commit**

```bash
git add docs/olimex-esp32-c6-evb.md docs/superpowers/specs/2026-08-09-olimex-c6-local-ui-design.md
git commit -m "docs: add olimex local ui bring-up guide"
```

---

### Task 7: Final Verification and PR Preparation

**Files:**

- No new source files; verifies whole branch.

**Interfaces:**

- Consumes all previous tasks.
- Produces final evidence for PR body.

- [ ] **Step 1: Inspect branch contents**

Run:

```bash
git status --short --branch
git diff --stat origin/main...HEAD
git log --oneline origin/main..HEAD
```

Expected: only intended source, test, and docs files are committed. `graphify-out/` may remain untracked but must not be committed.

- [ ] **Step 2: Run source formatting checks**

Run:

```bash
git diff --check origin/main...HEAD
npx prettier@3.5.3 --check "docs/**/*.md"
```

Expected: both pass.

- [ ] **Step 3: Run native tests**

Run:

```bash
cmake --build test/native/build && test/native/build/test_runner
```

Expected: all native suites pass, including `LocalSettingsMenu` and `KY-040 Decoder`.

- [ ] **Step 4: Run firmware builds**

Run:

```bash
venv/bin/pio run -e esp32dev
venv/bin/pio run -e norvi_ae01_r
venv/bin/pio run -e olimex_esp32_c6_evb
```

Expected: all three builds pass.

- [ ] **Step 5: Update PR body**

Use `gh pr edit` or `gh api --method PATCH` to include:

```markdown
## Summary

- add Olimex ESP32-C6-EVB board variant
- add configurable 320x240 SPI TFT local UI path
- add KY-040 encoder input handling
- add QR-code setup/diagnostic page support
- keep NORVI and esp32dev builds working

## Verification

- cmake --build test/native/build && test/native/build/test_runner
- venv/bin/pio run -e esp32dev
- venv/bin/pio run -e norvi_ae01_r
- venv/bin/pio run -e olimex_esp32_c6_evb
- git diff --check origin/main...HEAD
- npx prettier@3.5.3 --check "docs/\*_/_.md"
```

- [ ] **Step 6: Push branch**

```bash
git push
```

Expected: existing PR updates cleanly.
