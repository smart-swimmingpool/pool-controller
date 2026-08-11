#include "OlimexTftDisplay.hpp"

#if defined(OLIMEX_ESP32_C6_EVB) && defined(HAS_LOCAL_TFT_UI)

#include <qrcode.h>

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ST7789.h>

#include "Config.hpp"
#include "ConfigManager.hpp"
#include "Version.h"

namespace PoolController {
namespace {

Adafruit_ILI9341 tftIli9341(&SPI, PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_RST);
Adafruit_ST7789 tftSt7789(&SPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

Adafruit_SPITFT &activeTft() {
  return TFT_DRIVER_ST7789 ? static_cast<Adafruit_SPITFT &>(tftSt7789) : static_cast<Adafruit_SPITFT &>(tftIli9341);
}

std::uint16_t rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return TFT_DRIVER_ST7789 ? tftSt7789.color565(r, g, b) : tftIli9341.color565(r, g, b);
}

void drawHeader(const char *left, const char *right) {
  activeTft().setTextColor(rgb(255, 255, 255), rgb(0, 0, 0));
  activeTft().setTextSize(TFT_DISPLAY_SIZE_CLASS_COMPACT ? 1 : 2);
  activeTft().setCursor(8, 6);
  activeTft().print(left);
  activeTft().setCursor(240, 6);
  activeTft().print(right);
}

}  // namespace

bool OlimexTftDisplay::forceRedraw_{true};

void OlimexTftDisplay::begin() {
  SPI.begin(PIN_TFT_SCLK, PIN_TFT_MISO, PIN_TFT_MOSI, PIN_TFT_CS);
  if constexpr (TFT_DRIVER_ST7789) {
    tftSt7789.init(TFT_DISPLAY_WIDTH, TFT_DISPLAY_HEIGHT);
  } else {
    tftIli9341.begin(27000000);
  }
  activeTft().setRotation(1);
  activeTft().fillScreen(rgb(0, 0, 0));
  drawHeader("POOL", "BOOT");
  activeTft().setCursor(8, 40);
  activeTft().print("Starting...");
  forceRedraw_ = true;
}

void OlimexTftDisplay::drawPage(LocalUiPage page, LocalMenuItem menuItem) {
  if (!forceRedraw_) {
    return;
  }
  forceRedraw_ = false;
  activeTft().fillScreen(rgb(0, 0, 0));
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
  activeTft().setTextSize(TFT_DISPLAY_SIZE_CLASS_COMPACT ? 3 : 4);
  activeTft().setCursor(8, 38);
  activeTft().print("--.- C");
  activeTft().setTextSize(2);
  activeTft().setCursor(8, 92);
  activeTft().print("Pumpe: --");
  activeTft().drawFastHLine(0, 120, TFT_DISPLAY_WIDTH, rgb(64, 64, 64));
  drawHeader("SOLAR", "OK");
  activeTft().setTextSize(TFT_DISPLAY_SIZE_CLASS_COMPACT ? 3 : 4);
  activeTft().setCursor(8, 148);
  activeTft().print("--.- C");
  activeTft().setTextSize(2);
  activeTft().setCursor(8, 204);
  activeTft().print("Ventil: --");
}

void OlimexTftDisplay::drawMenu(LocalMenuItem menuItem) {
  drawHeader("MENU", "OK=Select");
  const char *items[] = {"Mode", "Pump", "Network", "QR Code", "Exit"};
  for (std::uint8_t i = 0; i < 5; ++i) {
    activeTft().setCursor(20, 42 + (i * 32));
    activeTft().setTextSize(2);
    activeTft().setTextColor(i == static_cast<std::uint8_t>(menuItem) ? rgb(0, 0, 0) : rgb(255, 255, 255),
      i == static_cast<std::uint8_t>(menuItem) ? rgb(255, 255, 0) : rgb(0, 0, 0));
    activeTft().print(items[i]);
  }
}

void OlimexTftDisplay::drawNetwork() {
  drawHeader("NETWORK", "STATUS");
  activeTft().setTextSize(2);
  activeTft().setCursor(8, 48);
  activeTft().print("WiFi/MQTT status");
}

void OlimexTftDisplay::drawSystem() {
  drawHeader("SYSTEM", FW_VERSION);
  activeTft().setTextSize(2);
  activeTft().setCursor(8, 48);
  activeTft().print("Heap / uptime");
}

void OlimexTftDisplay::drawQrCode() {
  drawHeader("QR", "WEB UI");
  const char *url = "http://pool-controller.local";
  QRCode qrcode;
  constexpr std::uint8_t kQrVersion{3};
  constexpr std::uint16_t kQrBufferSize{106};
  std::uint8_t qrcodeData[kQrBufferSize];
  qrcode_initText(&qrcode, qrcodeData, kQrVersion, ECC_LOW, url);
  const std::uint8_t scale = 6;
  const std::uint16_t offsetX = 70;
  const std::uint16_t offsetY = 35;
  activeTft().fillRect(offsetX - 8, offsetY - 8, (qrcode.size * scale) + 16, (qrcode.size * scale) + 16, rgb(255, 255, 255));
  for (std::uint8_t y = 0; y < qrcode.size; ++y) {
    for (std::uint8_t x = 0; x < qrcode.size; ++x) {
      if (qrcode_getModule(&qrcode, x, y)) {
        activeTft().fillRect(offsetX + (x * scale), offsetY + (y * scale), scale, scale, rgb(0, 0, 0));
      }
    }
  }
}

}  // namespace PoolController

#else

namespace PoolController {

bool OlimexTftDisplay::forceRedraw_{true};

void OlimexTftDisplay::begin() {}

void OlimexTftDisplay::drawPage(LocalUiPage, LocalMenuItem) {}

void OlimexTftDisplay::drawOverview() {}

void OlimexTftDisplay::drawMenu(LocalMenuItem) {}

void OlimexTftDisplay::drawNetwork() {}

void OlimexTftDisplay::drawSystem() {}

void OlimexTftDisplay::drawQrCode() {}

}  // namespace PoolController

#endif
