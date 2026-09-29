#include "qrcode.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace ricmoo {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "third_party/ricmoo_qrcode.inc"
#pragma GCC diagnostic pop
} // namespace ricmoo

static_assert(sizeof(QRCode) == sizeof(ricmoo::QRCode),
              "QRCode must mirror ricmoo::QRCode");

uint32_t qrcode_getBufferSize(uint8_t version) {
  return ricmoo::qrcode_getBufferSize(version);
}

int8_t qrcode_initText(QRCode *qrcode, uint8_t *modules, uint8_t version,
                       QrCodeEcc ecc, const char *data) {
  return ricmoo::qrcode_initText(reinterpret_cast<ricmoo::QRCode *>(qrcode),
                                 modules, version, static_cast<uint8_t>(ecc),
                                 data);
}

int qrcode_getModule(QRCode *qrcode, uint8_t x, uint8_t y) {
  return ricmoo::qrcode_getModule(reinterpret_cast<ricmoo::QRCode *>(qrcode), x,
                                  y)
             ? 1
             : 0;
}
