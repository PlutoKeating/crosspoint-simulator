#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../WString.h"
#include "portable_hash.h"

class MD5Builder {
public:
  MD5Builder() { memset(digest_, 0, sizeof(digest_)); }

  void begin() { ctx_.init(); }
  void add(const uint8_t *data, size_t len) { ctx_.update(data, len); }
  void add(const char *str) {
    if (str)
      add(reinterpret_cast<const uint8_t *>(str), strlen(str));
  }
  void calculate() { ctx_.finish(digest_); }

  String toString() const {
    char hex[33];
    for (int i = 0; i < 16; i++) {
      snprintf(hex + i * 2, 3, "%02x", digest_[i]);
    }
    return String(hex);
  }

private:
  sim_hash::Md5 ctx_{};
  uint8_t digest_[16];
};
