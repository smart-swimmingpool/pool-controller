// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file NorviOledDisplay.cpp
 * @brief OLED display implementation for the NORVI IIOT-AE01-R.
 *
 * Uses the Adafruit SSD1306 library with I2C (SDA=GPIO16, SCL=GPIO17).
 * Navigation: S1=UP, S2=DOWN, S3=CONFIRM. Auto-returns to MAIN after 60s idle.
 * Includes OLED burn-in mitigation via periodic 2px content shift.
 *
 * @note This file is only compiled when `NORVI_AE01_R` is defined.
 */

#ifdef NORVI_AE01_R

#include "NorviOledDisplay.hpp"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <qrcode.h>

#include "Config.hpp"
#include "NorviButtonHandler.hpp"
#include "Utils.hpp"
#include "DallasTemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "OperationModeNode.hpp"
#include "NetworkManager.hpp"
#include "SystemMonitor.hpp"
#include "TimeClientHelper.hpp"
#include "ConfigManager.hpp"

namespace PoolController {

// ── Forward declarations of global nodes (defined in PoolController.cpp) ───

extern DallasTemperatureNode solarTemperatureNode;
extern DallasTemperatureNode poolTemperatureNode;
extern RelayModuleNode poolPumpNode;
extern RelayModuleNode solarPumpNode;
extern OperationModeNode operationModeNode;

// ── File-scope helper forward declarations ────────────────────────────
// These are called from drawPage() which appears before their definition.
static void drawDegC(uint8_t textsize);
static void drawButtonHints();
static void drawProgressBar();

// ═══════════════════════════════════════════════════════════════════════════
// Static members
// ═══════════════════════════════════════════════════════════════════════════

NorviOledDisplay::Page NorviOledDisplay::currentPage_ = Page::MAIN;
uint32_t NorviOledDisplay::lastUpdateMs_ = 0;
bool NorviOledDisplay::forceRedraw_ = true;

uint32_t NorviOledDisplay::lastButtonPressMs_ = 0;

int8_t NorviOledDisplay::burnInDx_ = 0;
int8_t NorviOledDisplay::burnInDy_ = 0;
uint32_t NorviOledDisplay::lastBurnInShiftMs_ = 0;

NorviOledDisplay::SetupStep NorviOledDisplay::setupStep_ = SetupStep::IDLE;
uint8_t NorviOledDisplay::setupSelectedDev_ = 0;
bool NorviOledDisplay::setupSolarDone_ = false;
bool NorviOledDisplay::setupPoolDone_ = false;
uint8_t NorviOledDisplay::setupSolarAddr_[8] = {};
uint8_t NorviOledDisplay::setupPoolAddr_[8] = {};
bool NorviOledDisplay::setupRoleIsSolar_ = true;

bool NorviOledDisplay::firstBootDone_ = false;

NorviOledDisplay::MenuItem NorviOledDisplay::menuSelection_ = MenuItem::MODE;
bool NorviOledDisplay::menuActive_ = false;

// ── SSD1306 display instance (128×64, I2C, address 0x3C) ─────────────────

static Adafruit_SSD1306 display(128, 64, &Wire, -1);

/// Auto-return warning countdown (ms until return), 0 = no warning active.
/// Set by loop(), consumed by drawFooter().
static uint32_t autoReturnWarningMs = 0;

// ═══════════════════════════════════════════════════════════════════════════
// Coord helpers — apply burn-in offset to all drawing
// ═══════════════════════════════════════════════════════════════════════════

/// Set cursor with burn-in offset applied.
static void dspCursor(int16_t x, int16_t y) {
  display.setCursor(x + NorviOledDisplay::getBurnInDx(), y + NorviOledDisplay::getBurnInDy());
}

/// Fill rect with burn-in offset.
static void dspFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  display.fillRect(x + NorviOledDisplay::getBurnInDx(), y + NorviOledDisplay::getBurnInDy(), w, h, color);
}

/// Draw filled triangle with burn-in offset.
static void dspFillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  display.fillTriangle(x0 + NorviOledDisplay::getBurnInDx(), y0 + NorviOledDisplay::getBurnInDy(),
    x1 + NorviOledDisplay::getBurnInDx(), y1 + NorviOledDisplay::getBurnInDy(), x2 + NorviOledDisplay::getBurnInDx(),
    y2 + NorviOledDisplay::getBurnInDy(), color);
}

/// Draw circle with burn-in offset.
static void dspDrawCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
  display.drawCircle(x + NorviOledDisplay::getBurnInDx(), y + NorviOledDisplay::getBurnInDy(), r, color);
}

/// Horizontal line with burn-in offset.
static void dspHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  display.drawFastHLine(x + NorviOledDisplay::getBurnInDx(), y + NorviOledDisplay::getBurnInDy(), w, color);
}

/// Vertical line with burn-in offset.
static void dspVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  display.drawFastVLine(x + NorviOledDisplay::getBurnInDx(), y + NorviOledDisplay::getBurnInDy(), h, color);
}

/// Rounded rect with burn-in offset.
static void dspRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
  display.drawRoundRect(x + NorviOledDisplay::getBurnInDx(), y + NorviOledDisplay::getBurnInDy(), w, h, r, color);
}

/// Fill rounded rect with burn-in offset.
static void dspFillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
  display.fillRoundRect(x + NorviOledDisplay::getBurnInDx(), y + NorviOledDisplay::getBurnInDy(), w, h, r, color);
}

/// Fill circle with burn-in offset.
static void dspFillCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
  display.fillCircle(x + NorviOledDisplay::getBurnInDx(), y + NorviOledDisplay::getBurnInDy(), r, color);
}

/**
 * @brief Draw inverted (highlighted) text at (x, y).
 * Uses black-on-white within a measured bounding box so the inversion
 * exactly wraps the text.
 */
static void dspInvertedText(int16_t x, int16_t y, const char *text) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  dspFillRect(x1, y1, w, h, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  dspCursor(x, y);
  display.print(text);
  display.setTextColor(SSD1306_WHITE);
}

/// Overload for Flash strings.
static void dspInvertedText(int16_t x, int16_t y, const __FlashStringHelper *text) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  dspFillRect(x1, y1, w, h, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  dspCursor(x, y);
  display.print(text);
  display.setTextColor(SSD1306_WHITE);
}

// ═══════════════════════════════════════════════════════════════════════════
// Scrolling text helper — auto-scrolls horizontally if text exceeds maxWidth
// ═══════════════════════════════════════════════════════════════════════════
//
// Pauses 2 s at start, scrolls left at 25 px/s, pauses 1 s at end,
// rewinds at 50 px/s. Animation resets when page or text content changes.

static uint32_t scrollAnimStartMs_ = 0;
static int8_t scrollPhase_ = 0;      // 0=pause 1=scroll-L 2=pause 3=rewind
static int16_t scrollOffset_ = 0;

/**
 * @brief Draw text with horizontal scrolling if it exceeds @p maxWidth.
 *
 * The display driver clips at 0..127 automatically, so drawing at
 * x - offset achieves a hardware-accelerated left scroll without
 * manual clipping.
 */
static void drawScrollingText(int16_t x, int16_t y, const char *text, int16_t maxWidth) {
  if (!text || text[0] == '\0') return;

  int16_t x1, y1;
  uint16_t textW, h;
  display.getTextBounds(text, 0, y, &x1, &y1, &textW, &h);

  if (textW <= (uint16_t)maxWidth) {
    dspCursor(x, y);
    display.print(text);
    return;
  }

  // State tracking — read current page once
  static int16_t lastTextW = 0;
  static NorviOledDisplay::Page scrollPage_ = static_cast<NorviOledDisplay::Page>(0xFF);
  const auto curPage = NorviOledDisplay::getCurrentPage();

  // Reset animation on page or text width change
  if (textW != lastTextW) {
    scrollPhase_ = 0;
    scrollOffset_ = 0;
    scrollAnimStartMs_ = millis();
    lastTextW = textW;
  } else if (curPage != scrollPage_) {
    // Re-sync when navigating to a different page mid-scroll
    scrollPhase_ = 0;
    scrollOffset_ = 0;
    scrollAnimStartMs_ = millis();
    scrollPage_ = curPage;
  }

  const uint32_t now = millis();
  const int16_t scrollRange = textW - maxWidth;
  const uint32_t elapsed = now - scrollAnimStartMs_;

  switch (scrollPhase_) {
  case 0:  // Pause at start — full text visible entering from right
    scrollOffset_ = 0;
    if (elapsed >= 2000) { scrollPhase_ = 1; scrollAnimStartMs_ = now; }
    break;

  case 1:  // Scroll left at 25 px/s
    scrollOffset_ = static_cast<int16_t>(elapsed * 25 / 1000);
    if (scrollOffset_ >= scrollRange) {
      scrollOffset_ = scrollRange;
      scrollPhase_ = 2;
      scrollAnimStartMs_ = now;
    }
    break;

  case 2:  // Pause at end — last characters visible
    if (elapsed >= 1000) { scrollPhase_ = 3; scrollAnimStartMs_ = now; }
    break;

  case 3:  // Rewind right at 50 px/s
    scrollOffset_ = scrollRange - static_cast<int16_t>(elapsed * 50 / 1000);
    if (scrollOffset_ <= 0) {
      scrollOffset_ = 0;
      scrollPhase_ = 0;
      scrollAnimStartMs_ = now;
    }
    break;
  }

  dspCursor(x - scrollOffset_, y);
  display.print(text);
}

/// Overload for PROGMEM / Flash strings.
static void drawScrollingText(int16_t x, int16_t y, const __FlashStringHelper *text, int16_t maxWidth) {
  char buf[64];
  strncpy_P(buf, reinterpret_cast<PGM_P>(text), sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  drawScrollingText(x, y, buf, maxWidth);
}

// ═══════════════════════════════════════════════════════════════════════════
// begin() — Initialization
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::begin() {
  Serial.println("• NorviOledDisplay initializing on I2C GPIO16(SDA)/GPIO17(SCL)...");

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("✖ NorviOledDisplay: SSD1306 allocation failed — display disabled");
    return;
  }

  Serial.println("✓ NorviOledDisplay initialized (128×64, address 0x3C)");

  // ── Splash screen ──────────────────────────────────────────────────────
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  dspCursor(0, 0);
  display.println(F("Pool Controller"));
  display.println(F("v" FW_VERSION));
  display.println();
  display.println(F("NORVI AE01-R"));
  display.display();
  delay(1500);

  // ── Determine starting page based on first-boot state ──────────────────
  if (needsWiFiSetup()) {
    currentPage_ = Page::WIFI_SETUP;
    Serial.println("→ First boot: no WiFi configured — showing WIFI_SETUP page");
  } else if (needsSensorMapping()) {
    currentPage_ = Page::SENSOR_SETUP;
    Serial.println("→ First boot: sensors not mapped — showing SENSOR_SETUP page");
  } else {
    currentPage_ = Page::MAIN;
    firstBootDone_ = true;
  }

  lastButtonPressMs_ = millis();
  lastBurnInShiftMs_ = millis();
  forceRedraw_ = true;
}

// ═══════════════════════════════════════════════════════════════════════════
// loop() — Periodic update
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::loop() {
  const uint32_t now = millis();

  // ── Auto-return after idle timeout ───────────────────────────────────
  if (menuActive_ && (now - lastButtonPressMs_ >= 30000)) {
    // Menu: 30 s idle → close menu
    menuActive_ = false;
    forceRedraw_ = true;
  } else if (!isSetupActive() && currentPage_ != Page::MAIN
             && (now - lastButtonPressMs_ >= AUTO_RETURN_MS)) {
    // Info pages (non-MAIN): 60 s idle → MAIN
    currentPage_ = Page::MAIN;
    forceRedraw_ = true;
  }

  // ── Long-press progress: accelerate redraw during hold ──────────────
  float longPressProgress = NorviButtonHandler::getLongPressProgress();
  bool isLongPressing = (longPressProgress > 0.0f);

  // ── Auto-return warning (5s before) ─────────────────────────────────
  // Sets file-scope autoReturnWarningMs consumed by drawFooter()
  if (!menuActive_ && !isSetupActive() && currentPage_ != Page::MAIN) {
    uint32_t idleMs = now - lastButtonPressMs_;
    if (idleMs >= AUTO_RETURN_MS - 5000 && idleMs < AUTO_RETURN_MS) {
      autoReturnWarningMs = AUTO_RETURN_MS - idleMs;  // ms until return
      forceRedraw_ = true;
    } else {
      autoReturnWarningMs = 0;
    }
  } else {
    autoReturnWarningMs = 0;
  }

  // ── Throttle redraw rate (skip during long-press for smooth bar) ────
  if (!forceRedraw_ && !isLongPressing && (now - lastUpdateMs_ < UPDATE_INTERVAL_MS)) {
    return;
  }

  // ── Burn-in offset shift ────────────────────────────────────────────
  updateBurnInOffset();

  lastUpdateMs_ = now;
  forceRedraw_ = false;

  drawPage();
  drawProgressBar();
  display.display();
}

// ═══════════════════════════════════════════════════════════════════════════
// Navigation
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::previousPage() {
  uint8_t cur = static_cast<uint8_t>(currentPage_);
  if (cur == 0) {
    // Wrap: MAIN → WIFI_SETUP (skip SENSOR_SETUP in backward cycle)
    currentPage_ = Page::WIFI_SETUP;
  } else {
    currentPage_ = static_cast<Page>(cur - 1);
  }
  forceRedraw_ = true;
  lastButtonPressMs_ = millis();
}

void NorviOledDisplay::nextPage() {
  uint8_t next = static_cast<uint8_t>(currentPage_) + 1;
  if (next > maxNavPage()) {
    next = 0;
  }
  currentPage_ = static_cast<Page>(next);
  forceRedraw_ = true;
  lastButtonPressMs_ = millis();
}

void NorviOledDisplay::confirmAction() {
  lastButtonPressMs_ = millis();

  if (currentPage_ != Page::SENSOR_SETUP) {
    return;
  }

  // ── SENSOR_SETUP state machine ──────────────────────────────────────
  const uint8_t devCount = solarTemperatureNode.getDeviceCount();

  switch (setupStep_) {
  case SetupStep::IDLE:
    // Enter SELECT_SENSOR if sensors exist and not both already assigned
    if (devCount > 0 && !(setupSolarDone_ && setupPoolDone_)) {
      setupStep_ = SetupStep::SELECT_SENSOR;
      setupSelectedDev_ = 0;
      forceRedraw_ = true;
    }
    break;

  case SetupStep::SELECT_SENSOR:
    // Sensor selected → move to role selection
    if (setupSelectedDev_ < devCount) {
      setupStep_ = SetupStep::SELECT_ROLE;
      setupRoleIsSolar_ = !setupSolarDone_;  // Default to Solar if not done
      forceRedraw_ = true;
    }
    break;

  case SetupStep::SELECT_ROLE:
    // Apply assignment
    if (setupApplyAssignment()) {
      setupStep_ = SetupStep::IDLE;
      // Check if both done
      if (setupSolarDone_ && setupPoolDone_) {
        Serial.println("→ Both sensors assigned — save mapping via long-press S3");
      }
      forceRedraw_ = true;
    }
    break;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Action menu navigation
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::enterMenu() {
  menuActive_ = true;
  menuSelection_ = MenuItem::MODE;
  forceRedraw_ = true;
  lastButtonPressMs_ = millis();
}

void NorviOledDisplay::exitMenu() {
  menuActive_ = false;
  forceRedraw_ = true;
  lastButtonPressMs_ = millis();
}

void NorviOledDisplay::menuNext() {
  uint8_t cur = static_cast<uint8_t>(menuSelection_);
  cur = (cur + 1) % 3;  // MODE → PUMP → EXIT → MODE
  menuSelection_ = static_cast<MenuItem>(cur);
  forceRedraw_ = true;
  lastButtonPressMs_ = millis();
}

void NorviOledDisplay::menuPrevious() {
  uint8_t cur = static_cast<uint8_t>(menuSelection_);
  if (cur == 0) {
    cur = 2;  // wrap: MODE → EXIT
  } else {
    cur--;
  }
  menuSelection_ = static_cast<MenuItem>(cur);
  forceRedraw_ = true;
  lastButtonPressMs_ = millis();
}

// ═══════════════════════════════════════════════════════════════════════════
// drawPage() — Dispatch
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawPage() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // ── Action menu overlay ─────────────────────────────────────────────
  if (menuActive_) {
    drawMenuPage();
    drawButtonHints();
    drawFooter();
    return;
  }

  switch (currentPage_) {
  case Page::MAIN:
    drawMainPage();
    drawButtonHints();
    drawFooter();
    break;
  case Page::NETWORK:
    drawNetworkPage();
    drawButtonHints();
    drawFooter();
    break;
  case Page::SYSTEM:
    drawSystemPage();
    drawButtonHints();
    drawFooter();
    break;
  case Page::QRCODE:
    drawQrCodePage();
    if (!NetworkManager::isWiFiConnected()) {
      drawButtonHints();  // Only on non-connected QR page (hints overlap IP)
    }
    drawFooter();
    break;
  case Page::WIFI_SETUP:
    drawWiFiSetupPage();
    // Full page usage: no hints, no shared footer
    break;
  case Page::SENSOR_SETUP:
    drawSensorSetupPage();
    // No button hints — wizard has its own footer
    break;
  default:
    break;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Button hints bar (right side)
// ═══════════════════════════════════════════════════════════════════════════
//
// Physical button positions (right of OLED):
//   S1 (top)    = ▲ UP    ·   S2 (middle) = ▼ DOWN
//   S3 (bottom) = ■ OK
//
// Hints adapt based on current page and setup step.

static void drawButtonHints() {
  dspVLine(125, 14, 34, SSD1306_WHITE);

  // Draw button symbols (always present)
  dspFillTriangle(121, 14, 125, 20, 117, 20, SSD1306_WHITE);  // S1 ▲
  dspFillTriangle(117, 27, 125, 27, 121, 33, SSD1306_WHITE);   // S2 ▼

  // ── Menu mode hints ───────────────────────────────────────────────────
  if (NorviOledDisplay::isMenuActive()) {
    dspFillRoundRect(117, 40, 8, 6, 1, SSD1306_WHITE);
    dspCursor(99, 40);
    display.print(F("sel"));
    return;
  }

  const auto page = NorviOledDisplay::getCurrentPage();
  const bool selSens = NorviOledDisplay::isSelectSensorStep();
  const bool selRole = NorviOledDisplay::isSelectRoleStep();

  // ── S1 hint (top) ───────────────────────────────────────────────────────
  dspFillTriangle(121, 14, 125, 20, 117, 20, SSD1306_WHITE);
  dspCursor(86, 14);
  if (selSens || selRole) {
    display.print(F("up"));
  } else if (page == NorviOledDisplay::Page::MAIN) {
    display.print(F("wrap"));
  } else {
    display.print(F("prev"));
  }

  // ── S2 hint (middle) ────────────────────────────────────────────────────
  dspFillTriangle(117, 27, 125, 27, 121, 33, SSD1306_WHITE);
  dspCursor(83, 27);
  if (selSens || selRole) {
    display.print(F("dn"));
  } else {
    display.print(F("next"));
  }

  // ── S3 label (bottom) ──────────────────────────────────────────────────
  if (page == NorviOledDisplay::Page::MAIN) {
    dspFillRoundRect(117, 40, 8, 6, 1, SSD1306_WHITE);
    dspCursor(80, 40);
    display.print(F("menu"));
  } else if (page == NorviOledDisplay::Page::SENSOR_SETUP) {
    dspFillRoundRect(117, 40, 8, 6, 1, SSD1306_WHITE);
    dspCursor(80, 40);
    if (selSens) {
      display.print(F("select"));
    } else if (selRole) {
      display.print(F("assign"));
    } else {
      display.print(F("setup"));
    }
  }
  // Other info pages: no S3 action — hint is intentionally omitted
}

// ═══════════════════════════════════════════════════════════════════════════
// Action menu (MAIN → S3)
// ═══════════════════════════════════════════════════════════════════════════
//
//  ┌──────────────────────┬──────┐
//  │ Menu                 │      │
//  │──────────────────────│ S1 ▲│
//  │ ▓ Mode: auto         │ S2 ▼│
//  │   Pump: off          │ S3 ●│ sel
//  │   Exit menu          │      │
//  ├──────────────────────┴──────┤
//  │ AUTO  12:30  v4.0.2     1/5│
//  └─────────────────────────────┘

void NorviOledDisplay::drawMenuPage() {
  display.setTextSize(1);

  // ── Title ──────────────────────────────────────────────────────────────
  dspCursor(0, 0);
  display.print(F(" Menu"));
  dspHLine(0, 9, 128, SSD1306_WHITE);

  // ── Dynamic menu items ─────────────────────────────────────────────────
  char modeBuf[14];
  snprintf(modeBuf, sizeof(modeBuf), " Mode: %s",
           operationModeNode.getMode().c_str());

  char pumpBuf[14];
  snprintf(pumpBuf, sizeof(pumpBuf), " Pump: %s",
           poolPumpNode.getSwitch() ? "on " : "off");

  const char *exitItem = "  Exit menu";

  // ── Render items (12 px line height) ──────────────────────────────────
  static constexpr uint8_t Y0 = 14;
  static constexpr uint8_t LH = 12;

  // Item 1: Mode
  if (menuSelection_ == MenuItem::MODE) {
    dspInvertedText(4, Y0, modeBuf);
  } else {
    dspCursor(4, Y0);
    display.print(modeBuf);
  }

  // Item 2: Pump
  if (menuSelection_ == MenuItem::PUMP) {
    dspInvertedText(4, Y0 + LH, pumpBuf);
  } else {
    dspCursor(4, Y0 + LH);
    display.print(pumpBuf);
  }

  // Item 3: Exit
  if (menuSelection_ == MenuItem::EXIT) {
    dspInvertedText(4, Y0 + 2 * LH, exitItem);
  } else {
    dspCursor(4, Y0 + 2 * LH);
    display.print(exitItem);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Long-press progress bar overlay
// ═══════════════════════════════════════════════════════════════════════════

static void drawProgressBar() {
  float progress = NorviButtonHandler::getLongPressProgress();
  if (progress <= 0.0f) {
    return;
  }

  // Only show on SENSOR_SETUP page in IDLE state with both sensors done
  if (NorviOledDisplay::getCurrentPage() != NorviOledDisplay::Page::SENSOR_SETUP
      || NorviOledDisplay::isSetupActive()) {
    return;
  }

  // Bar dimensions (above footer line at y=55)
  static constexpr uint8_t BAR_X = 4;
  static constexpr uint8_t BAR_Y = 49;
  static constexpr uint8_t BAR_W = 120;
  static constexpr uint8_t BAR_H = 4;

  // Border
  dspRoundRect(BAR_X, BAR_Y, BAR_W, BAR_H, 1, SSD1306_WHITE);

  // Fill
  uint8_t fillW = static_cast<uint8_t>((BAR_W - 2) * progress);
  if (fillW > 0) {
    dspFillRect(BAR_X + 1, BAR_Y + 1, fillW, BAR_H - 2, SSD1306_WHITE);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// °C helper
// ═══════════════════════════════════════════════════════════════════════════

static void drawDegC(uint8_t textsize) {
  // Recover logical position (cursor already has burn-in applied via dspCursor)
  int16_t cx = display.getCursorX() - NorviOledDisplay::getBurnInDx();
  int16_t cy = display.getCursorY() - NorviOledDisplay::getBurnInDy();
  dspDrawCircle(cx + textsize + 1, cy + textsize, textsize, SSD1306_WHITE);
  dspCursor(cx + 2 * textsize + 4, cy);
  display.print('C');
}

// ═══════════════════════════════════════════════════════════════════════════
// Page 0 — MAIN: temperatures, mode, pump status
// ═══════════════════════════════════════════════════════════════════════════
//
//  ┌──────────────────────┐    ┌──────────────────────┐
//  │ ● 22.5°C             │    │  │ S1 ▲ nxt          │
//  │ Pool auto            │    │  │ S2 ▼ nxt          │
//  │ ○ 19.1°C             │    │  │ S3 ■ ok           │
//  │ Solar                │    └──────────────────────┘
//  └──────────────────────┘
//    Footer: AUTO  12:30  v2.1.0   1

void NorviOledDisplay::drawMainPage() {
  static constexpr uint8_t TX = 18;

  // ── Pump status indicators ──────────────────────────────────────────────
  if (poolPumpNode.getSwitch()) {
    dspFillCircle(8, 8, 4, SSD1306_WHITE);
  } else {
    dspDrawCircle(8, 8, 4, SSD1306_WHITE);
  }
  if (solarPumpNode.getSwitch()) {
    dspFillCircle(8, 36, 4, SSD1306_WHITE);
  } else {
    dspDrawCircle(8, 36, 4, SSD1306_WHITE);
  }

  // ── Pool temperature ────────────────────────────────────────────────────
  display.setTextSize(2);
  dspCursor(TX, 0);
  if (poolTemperatureNode.isSensorFound()) {
    char buf[8];
    Utils::floatToString(poolTemperatureNode.getTemperature(), buf, sizeof(buf), 1);
    display.print(buf);
    drawDegC(2);
  } else {
    display.print(F("--.-"));
    drawDegC(2);
  }

  // ── Pool label + mode ──────────────────────────────────────────────────
  display.setTextSize(1);
  dspCursor(TX, 18);
  display.print(F("Pool"));
  dspCursor(TX + 30, 18);
  display.print(operationModeNode.getMode());

  // ── Solar temperature ──────────────────────────────────────────────────
  display.setTextSize(2);
  dspCursor(TX, 28);
  if (solarTemperatureNode.isSensorFound()) {
    char buf[8];
    Utils::floatToString(solarTemperatureNode.getTemperature(), buf, sizeof(buf), 1);
    display.print(buf);
    drawDegC(2);
  } else {
    display.print(F("--.-"));
    drawDegC(2);
  }

  // ── Solar label ────────────────────────────────────────────────────────
  display.setTextSize(1);
  dspCursor(TX, 46);
  display.print(F("Solar"));
}

// ═══════════════════════════════════════════════════════════════════════════
// Page 1 — NETWORK: WiFi, IP, MQTT
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawNetworkPage() {
  dspCursor(0, 0);
  display.setTextSize(1);
  display.print(F(" Network Status"));
  dspHLine(0, 9, 128, SSD1306_WHITE);

  // ── WiFi SSID (scrolls if too long) ────────────────────────────────────
  dspCursor(0, 13);
  display.print(F("WiFi: "));
  if (NetworkManager::isWiFiConnected()) {
    if (NetworkManager::isApMode()) {
      display.print(F("AP MODE"));
    } else {
      drawScrollingText(36, 13, WiFi.SSID().c_str(), 128 - 36);
    }
  } else {
    display.print(F("---"));
  }

  // ── IP address ─────────────────────────────────────────────────────────
  dspCursor(0, 24);
  display.print(F("IP"));
  if (NetworkManager::isWiFiConnected()) {
    dspCursor(14, 24);
    display.print(WiFi.localIP().toString());
  } else {
    dspCursor(14, 24);
    display.print(F("---.---.---.---"));
  }

  // ── MQTT status ────────────────────────────────────────────────────────
  dspCursor(0, 35);
  display.print(F("MQTT: "));
  if (NetworkManager::isMqttConnected()) {
    display.print(F("CONNECTED"));
  } else if (NetworkManager::isWiFiConnected()) {
    display.print(F("connecting..."));
  } else {
    display.print(F("OFFLINE"));
  }

  // ── AP mode hint ───────────────────────────────────────────────────────
  if (NetworkManager::isApMode()) {
    dspCursor(0, 44);
    display.print(F("Browse to 192.168.4.1"));
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Page 2 — SYSTEM: uptime, heap, firmware
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawSystemPage() {
  dspCursor(0, 0);
  display.setTextSize(1);
  display.print(F(" System Info"));
  dspHLine(0, 9, 128, SSD1306_WHITE);

  // ── Uptime ─────────────────────────────────────────────────────────────
  dspCursor(0, 13);
  display.print(F("Up: "));
  char uptimeBuf[24];
  formatUptime(millis(), uptimeBuf, sizeof(uptimeBuf));
  display.print(uptimeBuf);

  // ── Free heap ──────────────────────────────────────────────────────────
  dspCursor(0, 24);
  display.print(F("Heap: "));
  display.print(SystemMonitor::getFreeHeap());
  display.print(F(" B"));

  // ── Firmware version ───────────────────────────────────────────────────
  dspCursor(0, 35);
  display.print(F("FW: " FW_VERSION));

  // ── Min free heap ──────────────────────────────────────────────────────
  dspCursor(0, 46);
  display.print(F("Min: "));
  display.print(SystemMonitor::getMinFreeHeap());
  display.print(F(" B"));
}

// ═══════════════════════════════════════════════════════════════════════════
// Page 3 — QRCODE: web interface access
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawQrCodePage() {
  display.setTextSize(1);

  // Build URL from current IP
  String url = "http://";
  if (NetworkManager::isWiFiConnected()) {
    url += WiFi.localIP().toString();
  } else if (NetworkManager::isApMode()) {
    url += F("192.168.4.1");
  } else {
    url += F("---.---.---.---");
  }
  url += '/';

  if (NetworkManager::isWiFiConnected()) {
    // ── WiFi connected: no QR (too small to scan), show IP prominently ──
    dspCursor(0, 0);
    display.print(F("Web Interface:"));
    dspCursor(0, 12);
    display.setTextSize(2);
    dspCursor(4, 24);
    display.print(WiFi.localIP().toString());
    display.setTextSize(1);
    dspCursor(0, 50);
    display.print(F("Open in your browser"));
  } else {
    // ── No WiFi: show QR code as large as possible ─────────────────────
    drawScrollingText(0, 0, url.c_str(), 128);

    // Generate QR code (version 1 = 21×21)
    static constexpr size_t kQrBufferSize = 75;
    uint8_t qrData[kQrBufferSize];
    QRCode qr;
    qrcode_initText(&qr, qrData, 1, 0, url.c_str());

    // Draw QR centered, max scale that fits (2px per module = 42×42)
    static constexpr uint8_t SCALE = 2;
    const uint8_t qrPx = qr.size * SCALE;
    const uint8_t xOff = (128 - qrPx) / 2;
    const uint8_t yOff = 10;

    for (uint8_t y = 0; y < qr.size; y++) {
      for (uint8_t x = 0; x < qr.size; x++) {
        if (qrcode_getModule(&qr, x, y)) {
          dspFillRect(xOff + x * SCALE, yOff + y * SCALE, SCALE, SCALE, SSD1306_WHITE);
        }
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Page 4 — WIFI_SETUP: captive portal info + QR
// ═══════════════════════════════════════════════════════════════════════════
//
// Shown on first boot when no WiFi credentials are configured.
// After WiFi is configured, this page is still accessible via navigation.

void NorviOledDisplay::drawWiFiSetupPage() {
  display.setTextSize(1);

  // ── Title ──────────────────────────────────────────────────────────────
  dspCursor(0, 0);
  display.print(F(" WiFi Setup"));
  dspHLine(0, 9, 128, SSD1306_WHITE);

  if (NetworkManager::isWiFiConnected()) {
    // ── WiFi already configured: no QR, just info ─────────────────────
    dspCursor(0, 20);
    display.print(F(" WiFi configured!"));
    dspCursor(0, 29);
    display.print(F(" IP: "));
    display.print(WiFi.localIP().toString());
    dspCursor(0, 46);
    display.print(F(" Open browser to IP"));
  } else {
    // ── No WiFi / AP mode: maximal QR code ─────────────────────────────
    String url = "http://";
    if (NetworkManager::isApMode()) {
      url += F("192.168.4.1");
      // Show AP SSID line above QR (scrolls if too long)
      dspCursor(0, 12);
      {
        display.print(F("AP: "));
        drawScrollingText(24, 12, WiFi.softAPSSID().c_str(), 128 - 24);
      }
    } else {
      url += F("192.168.4.1");
      dspCursor(0, 12);
      display.print(F("Connect to WiFi, then"));
    }
    url += '/';

    // Generate QR at max usable scale (2px = 42×42)
    static constexpr size_t kQrBufSize = 75;
    uint8_t qrBuf[kQrBufSize];
    QRCode qr;
    qrcode_initText(&qr, qrBuf, 1, 0, url.c_str());

    static constexpr uint8_t QR_SCALE = 2;
    const uint8_t qrPx = qr.size * QR_SCALE;
    const uint8_t qrX = (128 - qrPx) / 2;
    const uint8_t qrY = 20;

    for (uint8_t y = 0; y < qr.size; y++) {
      for (uint8_t x = 0; x < qr.size; x++) {
        if (qrcode_getModule(&qr, x, y)) {
          dspFillRect(qrX + x * QR_SCALE, qrY + y * QR_SCALE, QR_SCALE, QR_SCALE, SSD1306_WHITE);
        }
      }
    }

    // ── Instruction text below QR ────────────────────────────────────
    if (NetworkManager::isApMode()) {
      dspCursor(0, 56);
      display.print(F("Browse to 192.168.4.1"));
    } else {
      dspCursor(0, 56);
      display.print(F("browse to 192.168.4.1"));
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Page 5 — SENSOR_SETUP: address mapping wizard (two-step)
// ═══════════════════════════════════════════════════════════════════════════
//
// Step 1: SELECT_SENSOR — S1/S2 picks a sensor (inverted), S3 confirms
// Step 2: SELECT_ROLE  — S1/S2 picks Solar or Pool (inverted), S3 assigns

void NorviOledDisplay::drawSensorSetupPage() {
  display.setTextSize(1);
  dspCursor(0, 0);
  display.print(F(" Setup"));
  // Step indicator [1/2] or [2/2] when wizard is active
  if (setupStep_ != SetupStep::IDLE) {
    char stepBuf[8];
    uint8_t stepNum = (setupStep_ == SetupStep::SELECT_SENSOR) ? 1 : 2;
    snprintf(stepBuf, sizeof(stepBuf), " [%d/2]", stepNum);
    dspCursor(60, 0);
    display.print(stepBuf);
  }
  dspHLine(0, 9, 128, SSD1306_WHITE);

  const uint8_t devCount = solarTemperatureNode.getDeviceCount();

  if (devCount == 0) {
    dspCursor(0, 20);
    display.print(F(" No sensors found"));
    dspCursor(0, 35);
    display.print(F(" Check wiring"));
    drawSensorSetupFooter();
    return;
  }

  // ── Draw each detected device ──────────────────────────────────────────
  for (uint8_t i = 0; i < devCount && i < 2; i++) {
    const uint8_t y = 13 + i * 22;

    DeviceAddress addr;
    bool hasAddr = solarTemperatureNode.getDetectedDeviceAddress(i, addr);

    // Address line
    char adr[9] = "??";
    if (hasAddr) {
      snprintf(adr, sizeof(adr), "%02X%02X%02X%02X", addr[0], addr[1], addr[2], addr[3]);
    }

    // Check assignment
    bool isAssigned = false;
    const char *label = nullptr;
    if (setupSolarDone_ && hasAddr && memcmp(addr, setupSolarAddr_, 8) == 0) {
      isAssigned = true;
      label = "SOLAR";
    } else if (setupPoolDone_ && hasAddr && memcmp(addr, setupPoolAddr_, 8) == 0) {
      isAssigned = true;
      label = "POOL";
    }

    // Determine if this device is the currently selected one
    const bool isSelected = (setupStep_ != SetupStep::IDLE && i == setupSelectedDev_);

    // ── Render the device line ──────────────────────────────────────────
    char line[22];
    snprintf(line, sizeof(line), "%d: %s", i, adr);
    if (label) {
      // Append assignment badge
      strncat(line, " ", sizeof(line) - strlen(line) - 1);
      strncat(line, label, sizeof(line) - strlen(line) - 1);
    }

    if (isSelected && setupStep_ == SetupStep::SELECT_SENSOR) {
      // INVERTED highlight for the selected sensor
      dspInvertedText(6, y, line);
    } else if (isAssigned) {
      // Assigned sensor: show normally but dimmed / with badge
      dspCursor(6, y);
      display.print(line);
    } else {
      // Unassigned sensor
      dspCursor(6, y);
      display.print(line);
    }

    // ── Temperature line ────────────────────────────────────────────────
    float t = solarTemperatureNode.getDetectedDeviceTemperature(i);
    dspCursor(18, y + 10);
    if (!std::isnan(t)) {
      char buf[8];
      Utils::floatToString(t, buf, sizeof(buf), 1);
      display.print(buf);
    } else {
      display.print(F("--.-"));
    }
    drawDegC(1);

    // Arrow indicator on selected device
    if (isSelected && setupStep_ == SetupStep::SELECT_SENSOR) {
      dspCursor(100, y + 10);
      display.print(F("<-"));
    }
  }

  // ── Role selection overlay (replaces temperature area for 2nd step) ────
  if (setupStep_ == SetupStep::SELECT_ROLE) {
    // Draw a box over the temperature readouts
    dspRoundRect(2, 20, 90, 33, 3, SSD1306_WHITE);
    dspCursor(8, 24);
    display.print(F("Assign as:"));

    const char *solarTxt = "  Solar  ";
    const char *poolTxt = "  Pool   ";

    if (setupRoleIsSolar_) {
      dspInvertedText(8, 34, solarTxt);
      dspCursor(8, 44);
      display.print(poolTxt);
    } else {
      dspCursor(8, 34);
      display.print(solarTxt);
      dspInvertedText(8, 44, poolTxt);
    }
  }

  drawSensorSetupFooter();
}

// ═══════════════════════════════════════════════════════════════════════════
// SENSOR_SETUP footer
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawSensorSetupFooter() {
  dspHLine(0, 55, 128, SSD1306_WHITE);
  dspCursor(0, 56);

  switch (setupStep_) {
  case SetupStep::IDLE:
    if (setupSolarDone_ && setupPoolDone_) {
      display.print(F("Both set. Hold S3=Save"));
    } else if (solarTemperatureNode.getDeviceCount() > 0) {
      display.print(F("S3 to assign sensors"));
    } else {
      display.print(F("No sensors detected"));
    }
    break;
  case SetupStep::SELECT_SENSOR:
    display.print(F("S1/S2=pick  S3=ok"));
    break;
  case SetupStep::SELECT_ROLE:
    display.print(F("S1/S2=role  S3=save"));
    break;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Shared footer (info pages)
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawFooter() {
  dspHLine(0, 55, 128, SSD1306_WHITE);

  // ── Mode (MAIN page only) ─────────────────────────────────────────────
  display.setTextSize(1);
  if (currentPage_ == Page::MAIN) {
    dspCursor(0, 56);
    display.print(operationModeNode.getMode());
  }

  // ── Time ──────────────────────────────────────────────────────────────
  dspCursor(40, 56);
  time_t utc = getUtcTime();
  if (utc >= MIN_VALID_TIME) {
    TimeChangeRule *tcr = nullptr;
    time_t local = getTimeFor(getTimezoneIndex(), &tcr);
    struct tm *ti = localtime(&local);
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", ti->tm_hour, ti->tm_min);
    display.print(buf);
  } else {
    display.print(F("--:--"));
  }

  // Draw WiFi status icon at position 64
  if (NetworkManager::isWiFiConnected()) {
    // WiFi connected icon (3 vertical bars with decreasing height)
    dspFillRect(64, 56, 2, 8, SSD1306_WHITE);
    dspFillRect(67, 56, 2, 6, SSD1306_WHITE);
    dspFillRect(70, 56, 2, 4, SSD1306_WHITE);
    dspFillRect(73, 56, 2, 2, SSD1306_WHITE);
  } else {
    // WiFi disconnected icon (X over the WiFi symbol)
    dspFillRect(64, 56, 2, 8, SSD1306_WHITE);
    dspFillRect(67, 56, 2, 6, SSD1306_WHITE);
    dspFillRect(70, 56, 2, 4, SSD1306_WHITE);
    dspFillRect(73, 56, 2, 2, SSD1306_WHITE);
    // Draw X
    dspHLine(64, 60, 10, SSD1306_WHITE);
    dspVLine(70, 56, 4, SSD1306_WHITE);
  }

  // ── Firmware version ──────────────────────────────────────────────────
  dspCursor(78, 56);
  display.print(F("v" FW_VERSION));

  // ── Auto-return warning (5s window) ──────────────────────────────────
  // autoReturnWarningMs is set by loop() when idle approaches timeout
  {
    if (autoReturnWarningMs > 0) {
      uint8_t secs = (autoReturnWarningMs + 999) / 1000;
      // Overwrite version and page area with blink warning
      dspCursor(68, 56);
      display.print(F("→ MAIN "));
      display.print(secs);
      display.print('s');
    } else {
      // ── Page number (X/Y format) ────────────────────────────────────
      dspCursor(112, 56);
      uint8_t pageNum = static_cast<uint8_t>(currentPage_) + 1;
      uint8_t maxPage = static_cast<uint8_t>(Page::SENSOR_SETUP);
      char buf[8];
      snprintf(buf, sizeof(buf), "%d/%d", pageNum, maxPage);
      display.print(buf);
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Uptime formatter
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::formatUptime(uint32_t ms, char *buffer, size_t size) {
  const uint32_t totalSeconds = ms / 1000;
  const uint32_t days = totalSeconds / 86400;
  const uint32_t hours = (totalSeconds % 86400) / 3600;
  const uint32_t mins = (totalSeconds % 3600) / 60;

  if (days > 0) {
    snprintf(buffer, size, "%ud %uh %um", days, hours, mins);
  } else if (hours > 0) {
    snprintf(buffer, size, "%uh %um", hours, mins);
  } else {
    snprintf(buffer, size, "%um", mins > 0 ? mins : 1);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Burn-in mitigation: shift content in a 2×2 pixel pattern
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::updateBurnInOffset() {
  const uint32_t now = millis();
  if (now - lastBurnInShiftMs_ < BURN_IN_CYCLE_MS) {
    return;
  }
  lastBurnInShiftMs_ = now;

  // Cycle: (0,0) → (2,0) → (2,2) → (0,2) → (0,0)
  if (burnInDx_ == 0 && burnInDy_ == 0) {
    burnInDx_ = 2;
  } else if (burnInDx_ == 2 && burnInDy_ == 0) {
    burnInDy_ = 2;
  } else if (burnInDx_ == 2 && burnInDy_ == 2) {
    burnInDx_ = 0;
  } else {
    burnInDx_ = 0;
    burnInDy_ = 0;
  }

  forceRedraw_ = true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Sensor setup wizard — queries & actions
// ═══════════════════════════════════════════════════════════════════════════

bool NorviOledDisplay::needsWiFiSetup() {
  return ConfigManager::getWiFi().ssid.length() == 0;
}

bool NorviOledDisplay::needsSensorMapping() {
  // Check persistent NVS state rather than wizard-in-progress state.
  // If neither sensor has an address filter set, the user must assign them.
  return !solarTemperatureNode.hasAddressFilter() && !poolTemperatureNode.hasAddressFilter();
}

bool NorviOledDisplay::isMappingComplete() {
  return setupSolarDone_ && setupPoolDone_;
}

uint8_t NorviOledDisplay::getDetectedDeviceCount() {
  return solarTemperatureNode.getDeviceCount();
}

void NorviOledDisplay::getMapping(uint8_t solarAddr[8], uint8_t poolAddr[8]) {
  memcpy(solarAddr, setupSolarAddr_, 8);
  memcpy(poolAddr, setupPoolAddr_, 8);
}

void NorviOledDisplay::setupSelectPrevious() {
  // ── Cancel: at first item, go back to IDLE ──────────────────────────
  if (setupStep_ == SetupStep::SELECT_SENSOR && setupSelectedDev_ == 0) {
    setupStep_ = SetupStep::IDLE;
    setupSelectedDev_ = 0;
    forceRedraw_ = true;
    Serial.println("→ Sensor setup cancelled, back to IDLE");
    return;
  }

  uint8_t devCount = solarTemperatureNode.getDeviceCount();
  if (devCount > 2) {
    devCount = 2;
  }
  if (devCount == 0) {
    return;
  }
  if (setupSelectedDev_ == 0) {
    setupSelectedDev_ = devCount - 1;
  } else {
    setupSelectedDev_--;
  }
  forceRedraw_ = true;
}

void NorviOledDisplay::setupSelectNext() {
  uint8_t devCount = solarTemperatureNode.getDeviceCount();
  if (devCount > 2) {
    devCount = 2;
  }
  if (devCount == 0) {
    return;
  }
  setupSelectedDev_ = (setupSelectedDev_ + 1) % devCount;
  forceRedraw_ = true;
}

void NorviOledDisplay::setupToggleRole() {
  setupRoleIsSolar_ = !setupRoleIsSolar_;
  forceRedraw_ = true;
}

bool NorviOledDisplay::setupApplyAssignment() {
  DeviceAddress addr;
  if (!solarTemperatureNode.getDetectedDeviceAddress(setupSelectedDev_, addr)) {
    return false;
  }

  if (setupRoleIsSolar_) {
    // If this address is already assigned as Pool, clear that
    if (setupPoolDone_ && memcmp(addr, setupPoolAddr_, 8) == 0) {
      setupPoolDone_ = false;
      memset(setupPoolAddr_, 0, 8);
    }
    memcpy(setupSolarAddr_, addr, 8);
    setupSolarDone_ = true;
    Serial.println("→ Sensor assigned as Solar");
  } else {
    // If this address is already assigned as Solar, clear that
    if (setupSolarDone_ && memcmp(addr, setupSolarAddr_, 8) == 0) {
      setupSolarDone_ = false;
      memset(setupSolarAddr_, 0, 8);
    }
    memcpy(setupPoolAddr_, addr, 8);
    setupPoolDone_ = true;
    Serial.println("→ Sensor assigned as Pool");
  }

  forceRedraw_ = true;
  return true;
}

bool NorviOledDisplay::isAddressZero(const uint8_t addr[8]) {
  for (uint8_t i = 0; i < 8; i++) {
    if (addr[i] != 0) {
      return false;
    }
  }
  return true;
}

}  // namespace PoolController

#endif  // NORVI_AE01_R
