#pragma once
#include <stdint.h>

#define SSD1306_WHITE 1
#define SSD1306_BLACK 0

class Adafruit_SSD1306 {
public:
  Adafruit_SSD1306(int, int, void*, int, int, int) {}
  bool begin(uint8_t, uint8_t) { return true; }
  void display() {}
  void clearDisplay() {}
  void setCursor(int, int) {}
  void setTextSize(int) {}
  void setTextColor(uint16_t) {}
  void print(const char*) {}
  void print(int) {}
  void println(const char*) {}
  void println(int) {}
  void drawPixel(int, int, uint16_t) {}
  void drawLine(int, int, int, int, uint16_t) {}
  void drawRect(int, int, int, int, uint16_t) {}
  void fillRect(int, int, int, int, uint16_t) {}
  void drawCircle(int, int, int, uint16_t) {}
  void fillCircle(int, int, int, uint16_t) {}
  void drawBitmap(int, int, const uint8_t*, int, int, uint16_t) {}
  void drawXBitmap(int, int, const uint8_t*, int, int, uint16_t) {}
  void ssd1306_commandList(const uint8_t*) {}
  int width() { return 128; }
  int height() { return 64; }
  uint8_t ssd1306_128x64_i2c;
};
