#include "OlimexTftDisplay.hpp"

#if defined(OLIMEX_ESP32_C6_EVB) && defined(HAS_LOCAL_TFT_UI)

#include <qrcode.h>

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ST7789.h>

#include "Config.hpp"
#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "Nodes.hpp"
#include "Utils.hpp"
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

void drawHeader(const char *left, const char *right, std::uint16_t y) {
  activeTft().setTextColor(rgb(255, 255, 255), rgb(0, 0, 0));
  activeTft().setTextSize(TFT_DISPLAY_SIZE_CLASS_COMPACT ? 1 : 2);
  activeTft().setCursor(8, y);
  activeTft().print(left);
  activeTft().setCursor(240, y);
  activeTft().print(right);
}

void drawStatusLine(std::uint16_t x, std::uint16_t y, const char *label, const char *value) {
  activeTft().setTextSize(2);
  activeTft().setCursor(x, y);
  activeTft().print(label);
  activeTft().print(value);
}

}  // namespace

bool OlimexTftDisplay::forceRedraw_{true};

void OlimexTftDisplay::begin() {
  SPI.begin(PIN_TFT_SCLK, PIN_TFT_MISO, PIN_TFT_MOSI, PIN_TFT_CS);
  if constexpr (TFT_DRIVER_ST7789) {
    tftSt7789.init(TFT_DISPLAY_HEIGHT, TFT_DISPLAY_WIDTH);
  } else {
    tftIli9341.begin(27000000);
  }
  activeTft().setRotation(1);
  activeTft().fillScreen(rgb(0, 0, 0));
  drawHeader("POOL", "BOOT", 6);
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
  drawHeader("POOL", operationModeNode.getMode().c_str(), 6);
  char buf[16];
  Utils::floatToString(poolTemperatureNode.getTemperature(), buf, sizeof(buf), 1);
  activeTft().setTextSize(TFT_DISPLAY_SIZE_CLASS_COMPACT ? 3 : 4);
  activeTft().setCursor(8, 38);
  activeTft().print(buf);
  activeTft().print(" C");
  drawStatusLine(8, 92, "Pump: ", poolPumpNode.getSwitch() ? "ON" : "OFF");
  activeTft().drawFastHLine(0, 120, TFT_DISPLAY_WIDTH, rgb(64, 64, 64));
  drawHeader("SOLAR", solarPumpNode.getSwitch() ? "ON" : "OFF", 126);
  Utils::floatToString(solarTemperatureNode.getTemperature(), buf, sizeof(buf), 1);
  activeTft().setTextSize(TFT_DISPLAY_SIZE_CLASS_COMPACT ? 3 : 4);
  activeTft().setCursor(8, 148);
  activeTft().print(buf);
  activeTft().print(" C");
  drawStatusLine(8, 204, "Valve: ", solarPumpNode.getSwitch() ? "OPEN" : "CLOSED");
}

void OlimexTftDisplay::drawMenu(LocalMenuItem menuItem) {
  drawHeader("MENU", "OK=Select", 6);
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
  drawHeader("NETWORK", NetworkManager::isApMode() ? "AP" : "STA", 6);
  char buf[32];
  activeTft().setTextSize(2);
  activeTft().setCursor(8, 48);
  activeTft().print(NetworkManager::isWiFiConnected() ? "WiFi: ON " : "WiFi: OFF");
  activeTft().print("MQTT: ");
  activeTft().print(NetworkManager::isMqttConnected() ? "ON" : "OFF");
  activeTft().setCursor(8, 80);
  snprintf(buf, sizeof(buf), "RSSI: %d dBm", NetworkManager::getWiFiRSSI());
  activeTft().print(buf);
  activeTft().setCursor(8, 112);
  activeTft().print("IP: ");
  activeTft().print(NetworkManager::getLocalIP());
}

void OlimexTftDisplay::drawSystem() {
  drawHeader("SYSTEM", FW_VERSION, 6);
  char buf[32];
  activeTft().setTextSize(2);
  activeTft().setCursor(8, 48);
  snprintf(buf, sizeof(buf), "Heap: %u B", ESP.getFreeHeap());
  activeTft().print(buf);
  activeTft().setCursor(8, 80);
  snprintf(buf, sizeof(buf), "Uptime: %lus", millis() / 1000UL);
  activeTft().print(buf);
}

void OlimexTftDisplay::drawQrCode() {
  drawHeader("QR", "WEB UI", 6);
  const char *url = "http://pool-controller.local";
  QRCode qrcode;
  constexpr std::uint8_t kQrVersion{3};
  constexpr std::uint16_t kQrBufferSize{106};
  std::uint8_t qrcodeData[kQrBufferSize];
  qrcode_initText(&qrcode, qrcodeData, kQrVersion, ECC_LOW, url);
  const std::uint8_t scale = 6;
  const std::uint16_t offsetX = 70;
  const std::uint16_t offsetY = 35;
  const std::uint16_t quiet = static_cast<std::uint16_t>(4U * scale);
  activeTft().fillRect(offsetX - quiet, offsetY - quiet, (qrcode.size * scale) + (2U * quiet),
    (qrcode.size * scale) + (2U * quiet), rgb(255, 255, 255));
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
