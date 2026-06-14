#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
  uint8_t *moduleCount;
  uint8_t **modules;
} QRCode;

int qrcode_initText(QRCode *qrcode, uint8_t *modules, uint8_t version, const char *data) { return 0; }
void qrcode_free(QRCode *qrcode) {}
void qrcode_print(const QRCode *qrcode) {}
