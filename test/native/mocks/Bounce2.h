#pragma once
#include <stdint.h>

class Bounce {
public:
  void attach(int pin, int mode) {}
  void interval(uint16_t ms) {}
  bool update() { return false; }
  bool fell() { return false; }
  bool rose() { return false; }
  bool read() { return false; }
  int getPin() { return 0; }
};
