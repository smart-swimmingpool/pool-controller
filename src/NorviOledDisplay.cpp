// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file NorviOledDisplay.cpp
 * @brief OLED display implementation for the NORVI IIOT-AE01-R.
 *
 * Uses the Adafruit SSD1306 library with I2C (SDA=GPIO16, SCL=GPIO17).
 * Displays system information on three switchable pages.
 * The display updates at most every 2 seconds to reduce I2C traffic.
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

// ── Static members ─────────────────────────────────────────────────────────

NorviOledDisplay::Page NorviOledDisplay::currentPage_ = Page::MAIN;
uint32_t NorviOledDisplay::lastUpdateMs_ = 0;
bool NorviOledDisplay::forceRedraw_ = true;

bool NorviOledDisplay::setupActive_ = false;
uint8_t NorviOledDisplay::setupSelectedDev_ = 0;
bool NorviOledDisplay::setupSolarDone_ = false;
bool NorviOledDisplay::setupPoolDone_ = false;
uint8_t NorviOledDisplay::setupSolarAddr_[8] = {};
uint8_t NorviOledDisplay::setupPoolAddr_[8] = {};

// ── SSD1306 display instance (128×64, I2C, address 0x3C) ─────────────────

static Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::begin() {
  Serial.println("• NorviOledDisplay initializing on I2C GPIO16(SDA)/GPIO17(SCL)...");

  // Initialize I2C with NORVI-specific pins
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("✖ NorviOledDisplay: SSD1306 allocation failed — display disabled");
    return;
  }

  Serial.println("✓ NorviOledDisplay initialized (128×64, address 0x3C)");

  // Show splash screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Pool Controller"));
  display.println(F("v" FW_VERSION));
  display.println();
  display.println(F("NORVI AE01-R"));
  display.display();
  delay(1500);

  // Always start on the main temperature overview page.
  currentPage_ = Page::MAIN;
  forceRedraw_ = true;
}

// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::loop() {
  const uint32_t now = millis();

  if (!forceRedraw_ && (now - lastUpdateMs_ < UPDATE_INTERVAL_MS)) {
    return;
  }

  lastUpdateMs_ = now;
  forceRedraw_ = false;

  drawPage();
  display.display();
}

// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::nextPage() {
  uint8_t next = static_cast<uint8_t>(currentPage_) + 1;
  if (next > MAX_NAV_PAGE) {
    next = 0;
  }
  currentPage_ = static_cast<Page>(next);
  forceRedraw_ = true;
}

void NorviOledDisplay::prevPage() {
  uint8_t prev = static_cast<uint8_t>(currentPage_);
  if (prev == 0) {
    prev = MAX_NAV_PAGE;
  } else {
    prev--;
  }
  currentPage_ = static_cast<Page>(prev);
  forceRedraw_ = true;
}

// ═══════════════════════════════════════════════════════════════════════════

// Forward declarations for file-scope helpers used by the page drawing functions.
static void drawButtonHints();
static void drawDegC(uint8_t textsize);

void NorviOledDisplay::drawPage() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

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
    drawButtonHints();
    drawFooter();
    break;
  case Page::SENSOR_SETUP:
    drawSensorSetupPage();
    break;
  default:
    break;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Helper: °C unit after a temperature number, drawn as a small circle
// (sharper than the font's (char)247 glyph at text size 2, and avoids
// encoding issues with UTF-8 ° in string literals).
// ═══════════════════════════════════════════════════════════════════════════

/// Draw consistent right-side button hints on every info page.
/// Physical button positions (right of OLED):
///   S1 (top)    = ▲ next page    ·   S2 (middle) = ▼ toggle pump
///   S3 (bottom) = ■ cycle mode
static void drawButtonHints() {
  // ── Vertical bar flush-right, icons touching bar, text right-aligned ──
  display.drawFastVLine(125, 14, 34, SSD1306_WHITE);  // y=14..48

  // S1 ▲ nxt (top button) ─────────────────────────────────────────────
  display.fillTriangle(121, 14, 125, 20, 117, 20, SSD1306_WHITE);
  display.setCursor(99, 14);
  display.print(F("nxt"));

  // S2 ▼ tog (middle button) ──────────────────────────────────────────
  display.fillTriangle(117, 27, 125, 27, 121, 33, SSD1306_WHITE);
  display.setCursor(99, 27);
  display.print(F("tog"));

  // S3 ■ mod (bottom button) ──────────────────────────────────────────
  display.fillRoundRect(117, 40, 8, 6, 1, SSD1306_WHITE);
  display.setCursor(99, 40);
  display.print(F("mod"));
}

static void drawDegC(uint8_t textsize) {
  int16_t cx = display.getCursorX();
  int16_t cy = display.getCursorY();
  display.drawCircle(cx + textsize + 1, cy + textsize, textsize, SSD1306_WHITE);
  display.setCursor(cx + 2 * textsize + 4, cy);
  display.print('C');
}

// ═══════════════════════════════════════════════════════════════════════════
// Page 0 — Main: temperatures, operation mode, status indicator
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawMainPage() {
  //
  //  Left column (content)          Right column (button hints)
  //  ┌────────────────────┐         ┌──────────────────────────┐
  //  │ ● 22.5°C           │         │  │ S1 ▲ nxt             │
  //  │ Pool auto          │         │  │ S2 ▼ tog             │
  //  │ ○ 19.1°C           │         │  │ S3 ■ mod             │
  //  │ Solar              │         └──────────────────────────┘
  //  └────────────────────┘
  //    Footer: AUTO  12:30  v2.1.0   1
  //
  static constexpr uint8_t TX = 18;

  // ── Pump status indicator (filled ● = ON, outline ○ = OFF) ────────────
  if (poolPumpNode.getSwitch()) {
    display.fillCircle(8, 8, 4, SSD1306_WHITE);
  } else {
    display.drawCircle(8, 8, 4, SSD1306_WHITE);
  }
  if (solarPumpNode.getSwitch()) {
    display.fillCircle(8, 36, 4, SSD1306_WHITE);
  } else {
    display.drawCircle(8, 36, 4, SSD1306_WHITE);
  }

  // ── Pool temperature (size 2) ──────────────────────────────────────────
  display.setTextSize(2);
  display.setCursor(TX, 0);
  if (poolTemperatureNode.isSensorFound()) {
    char buf[8];
    Utils::floatToString(poolTemperatureNode.getTemperature(), buf, sizeof(buf), 1);
    display.print(buf);
    drawDegC(2);
  } else {
    display.print(F("--.-"));
    drawDegC(2);
  }

  // ── Pool label + operation mode ────────────────────────────────────────
  display.setTextSize(1);
  display.setCursor(TX, 18);
  display.print(F("Pool"));
  display.setCursor(TX + 30, 18);
  display.print(operationModeNode.getMode());

  // ── Solar temperature (size 2) ─────────────────────────────────────────
  display.setTextSize(2);
  display.setCursor(TX, 28);
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
  display.setCursor(TX, 46);
  display.print(F("Solar"));
}

// ═══════════════════════════════════════════════════════════════════════════
// Page 1 — Network: WiFi, IP, MQTT
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawNetworkPage() {
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print(F(" Network Status"));
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  // ── WiFi SSID ─────────────────────────────────────────────────────────
  display.setCursor(0, 13);
  display.print(F("WiFi: "));
  if (NetworkManager::isWiFiConnected()) {
    if (NetworkManager::isApMode()) {
      display.print(F("AP MODE"));
    } else {
      String ssid = WiFi.SSID();
      if (ssid.length() > 9) {
        ssid = ssid.substring(0, 9);
      }
      display.print(ssid);
    }
  } else {
    display.print(F("---"));
  }

  // ── IP address ────────────────────────────────────────────────────────
  display.setCursor(0, 24);
  display.print(F("IP"));
  if (NetworkManager::isWiFiConnected()) {
    display.setCursor(14, 24);
    display.print(WiFi.localIP().toString());
  } else {
    display.setCursor(14, 24);
    display.print(F("---.---.---.---"));
  }

  // ── MQTT status ───────────────────────────────────────────────────────
  display.setCursor(0, 35);
  display.print(F("MQTT: "));
  if (NetworkManager::isMqttConnected()) {
    display.print(F("CONNECTED"));
  } else if (NetworkManager::isWiFiConnected()) {
    display.print(F("connecting..."));
  } else {
    display.print(F("OFFLINE"));
  }

  // ── AP mode hint ──────────────────────────────────────────────────────
  if (NetworkManager::isApMode()) {
    display.setCursor(0, 44);
    display.print(F("Browse to 192.168.4.1"));
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Page 2 — System: uptime, free heap, firmware version
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawSystemPage() {
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print(F(" System Info"));
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  // ── Uptime ────────────────────────────────────────────────────────────
  display.setCursor(0, 13);
  display.print(F("Up: "));
  char uptimeBuf[24];
  formatUptime(millis(), uptimeBuf, sizeof(uptimeBuf));
  display.print(uptimeBuf);

  // ── Free heap ─────────────────────────────────────────────────────────
  display.setCursor(0, 24);
  display.print(F("Heap: "));
  display.print(SystemMonitor::getFreeHeap());
  display.print(F(" B"));

  // ── Firmware version ──────────────────────────────────────────────────
  display.setCursor(0, 35);
  display.print(F("FW: " FW_VERSION));

  // ── Min free heap (low watermark) ────────────────────────────────────
  display.setCursor(0, 46);
  display.print(F("Min: "));
  display.print(SystemMonitor::getMinFreeHeap());
  display.print(F(" B"));
}

// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::formatUptime(uint32_t ms, char *buffer, size_t size) {
  const uint32_t totalSeconds = ms / 1000;
  const uint32_t days  = totalSeconds / 86400;
  const uint32_t hours = (totalSeconds % 86400) / 3600;
  const uint32_t mins  = (totalSeconds % 3600) / 60;

  if (days > 0) {
    snprintf(buffer, size, "%ud %uh %um", days, hours, mins);
  } else if (hours > 0) {
    snprintf(buffer, size, "%uh %um", hours, mins);
  } else {
    snprintf(buffer, size, "%um", mins > 0 ? mins : 1);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Shared footer — time + version + page number on every info page
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawFooter() {
  display.drawFastHLine(0, 55, 128, SSD1306_WHITE);

  // ── Mode (MAIN page only) ─────────────────────────────────────────────
  display.setTextSize(1);
  if (currentPage_ == Page::MAIN) {
    display.setCursor(0, 56);
    display.print(operationModeNode.getMode());
  }

  // ── Time ──────────────────────────────────────────────────────────────
  display.setCursor(40, 56);
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

  // ── Firmware version ──────────────────────────────────────────────────
  display.setCursor(76, 56);
  display.print(F("v" FW_VERSION));

  // ── Page number ───────────────────────────────────────────────────────
  display.setCursor(118, 56);
  display.print(static_cast<uint8_t>(currentPage_) + 1);
}

// ═══════════════════════════════════════════════════════════════════════════
// Page 4 — QR Code: web interface access
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawQrCodePage() {
  //
  //  ┌──────────────────────────────────┐
  //  │ http://192.168.1.100/            │  ← URL
  //  │                                  │
  //  │      ████████████████            │  ← QR code (21×21 × 2px)
  //  │      ██  ████  ████  ██          │
  //  │      ██  ████  ████  ██          │
  //  │      ████████████████            │
  //  │      ██  ██  ████  ██            │
  //  │      ████████████████            │
  //  │       Quick-Access               │
  //  └──────────────────────────────────┘
  //

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

  // Show URL at top (truncate if needed)
  display.setCursor(0, 0);
  if (url.length() <= 20) {
    display.print(url);
  } else {
    display.print(url.substring(0, 20));
  }

  // Generate QR code (version 1 = 21×21 modules)
  static constexpr size_t kQrBufferSize = 75;
  uint8_t qrData[kQrBufferSize];
  QRCode qr;
  qrcode_initText(&qr, qrData, 1, 0, url.c_str());

  // Draw QR code centered, at 2px per module
  static constexpr uint8_t SCALE = 2;
  const uint8_t qrPx = qr.size * SCALE;
  const uint8_t xOff = (128 - qrPx) / 2;
  const uint8_t yOff = 8;  // below URL — QR code ends at y=49, footer at y=55

  for (uint8_t y = 0; y < qr.size; y++) {
    for (uint8_t x = 0; x < qr.size; x++) {
      if (qrcode_getModule(&qr, x, y)) {
        display.fillRect(xOff + x * SCALE, yOff + y * SCALE, SCALE, SCALE, SSD1306_WHITE);
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Page 4 — Sensor Setup: address mapping wizard
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::drawSensorSetupPage() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F(" Sensor Setup"));
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  const uint8_t devCount = solarTemperatureNode.getDeviceCount();

  if (devCount == 0) {
    display.setCursor(0, 20);
    display.print(F(" No sensors found"));
    display.setCursor(0, 35);
    display.print(F(" Check wiring"));
    return;
  }

  // Show each detected device (max 2 for NORVI shared bus)
  for (uint8_t i = 0; i < devCount && i < 2; i++) {
    // ── Device line ─────────────────────────────────────────────────────
    uint8_t y = 13 + i * 22;

    // Selection indicator (only show selection in active setup mode)
    if (setupActive_ && i == setupSelectedDev_) {
      display.setCursor(0, y);
      display.print('>');
    }
    display.setCursor(6, y);
    display.print(i);
    display.print(": ");

    // Address (truncated to first 8 hex chars for space)
    DeviceAddress addr;
    if (solarTemperatureNode.getDetectedDeviceAddress(i, addr)) {
      char adr[9];
      snprintf(adr, sizeof(adr), "%02X%02X%02X%02X", addr[0], addr[1], addr[2], addr[3]);
      display.print(adr);
    } else {
      display.print(F("??"));
    }

    // Assignment badge
    bool isSolar = false, isPool = false;
    if (setupSolarDone_ && memcmp(addr, setupSolarAddr_, 8) == 0) {
      isSolar = true;
    }
    if (setupPoolDone_ && memcmp(addr, setupPoolAddr_, 8) == 0) {
      isPool = true;
    }
    if (isSolar) {
      display.setCursor(80, y);
      display.print(F("SOLAR"));
    } else if (isPool) {
      display.setCursor(80, y);
      display.print(F("POOL"));
    }

    // ── Temperature line ────────────────────────────────────────────────
    float t = solarTemperatureNode.getDetectedDeviceTemperature(i);
    display.setCursor(18, y + 10);
    if (!std::isnan(t)) {
      char buf[8];
      Utils::floatToString(t, buf, sizeof(buf), 1);
      display.print(buf);
      drawDegC(1);
    } else {
      display.print(F("--.-"));
      drawDegC(1);
    }

    // "←" indicator on selected device (only in active setup)
    if (setupActive_ && i == setupSelectedDev_) {
      display.setCursor(100, y + 10);
      display.print(F("←"));
    }
  }

  // ── Footer / help line ────────────────────────────────────────────────
  display.drawFastHLine(0, 55, 128, SSD1306_WHITE);
  display.setCursor(0, 56);

  if (setupActive_) {
    if (setupSolarDone_ && setupPoolDone_) {
      display.print(F("B2 long=Save&Rbt"));
    } else {
      display.print(F("B2=Solar B3=Pool"));
    }
  } else {
    display.print(F("Hold B1 to setup"));
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Sensor setup wizard API
// ═══════════════════════════════════════════════════════════════════════════

void NorviOledDisplay::enterSetupMode() {
  setupActive_ = true;
  setupSelectedDev_ = 0;
  setupSolarDone_ = false;
  setupPoolDone_ = false;
  memset(setupSolarAddr_, 0, 8);
  memset(setupPoolAddr_, 0, 8);
  currentPage_ = Page::SENSOR_SETUP;
  forceRedraw_ = true;
}

void NorviOledDisplay::exitSetupMode() {
  setupActive_ = false;
  currentPage_ = Page::MAIN;
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

bool NorviOledDisplay::setupAssignAsSolar() {
  DeviceAddress addr;
  if (!solarTemperatureNode.getDetectedDeviceAddress(setupSelectedDev_, addr)) {
    return false;
  }
  // If this address is already assigned as Pool, clear that assignment
  if (setupPoolDone_ && memcmp(addr, setupPoolAddr_, 8) == 0) {
    setupPoolDone_ = false;
    memset(setupPoolAddr_, 0, 8);
  }
  memcpy(setupSolarAddr_, addr, 8);
  setupSolarDone_ = true;
  forceRedraw_ = true;
  return true;
}

bool NorviOledDisplay::setupAssignAsPool() {
  DeviceAddress addr;
  if (!solarTemperatureNode.getDetectedDeviceAddress(setupSelectedDev_, addr)) {
    return false;
  }
  // If this address is already assigned as Solar, clear that assignment
  if (setupSolarDone_ && memcmp(addr, setupSolarAddr_, 8) == 0) {
    setupSolarDone_ = false;
    memset(setupSolarAddr_, 0, 8);
  }
  memcpy(setupPoolAddr_, addr, 8);
  setupPoolDone_ = true;
  forceRedraw_ = true;
  return true;
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
