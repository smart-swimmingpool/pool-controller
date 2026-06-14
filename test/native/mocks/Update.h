#pragma once
#include <stdint.h>
#include <stddef.h>
#include "Arduino.h"

#define UPDATE_SIZE_UNKNOWN 0
#define UPLOAD_FILE_START 0
#define UPLOAD_FILE_WRITE 1
#define UPLOAD_FILE_END 2

struct HTTPUpload {
  int status = 0;
  uint8_t* buf = nullptr;
  size_t currentSize = 0;
  size_t totalSize = 0;
  String filename;
};

class UpdateClass {
public:
  bool begin(size_t size = UPDATE_SIZE_UNKNOWN, int command = 0) { return true; }
  bool end(bool evenIfRemaining = false) { return true; }
  size_t write(const uint8_t* data, size_t len) { return len; }
  bool hasError() { return false; }
  void printError(Print &p) {}
  void printError(int) {}
  void printError(SerialClass &s) {}
  String errorString() { return String(""); }
  void abort() {}
};
static UpdateClass Update;
