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
