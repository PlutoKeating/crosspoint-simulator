#pragma once

#include <cstdint>

// QR encoder for the simulator, API-compatible with ricmoo/QRCode as used by
// the firmware's QrUtils. Backed by the vendored MIT implementation in
// third_party/ so binding codes render as scannable QR codes.
enum QrCodeEcc { ECC_LOW = 0, ECC_MEDIUM, ECC_QUARTILE, ECC_HIGH };

// Same layout as ricmoo's QRCode struct.
class QRCode {
public:
  uint8_t version = 0;
  uint8_t size = 0;
  uint8_t ecc = 0;
  uint8_t mode = 0;
  uint8_t mask = 0;
  uint8_t *modules = nullptr;
};

uint32_t qrcode_getBufferSize(uint8_t version);
int8_t qrcode_initText(QRCode *qrcode, uint8_t *modules, uint8_t version,
                       QrCodeEcc ecc, const char *data);
int qrcode_getModule(QRCode *qrcode, uint8_t x, uint8_t y);
