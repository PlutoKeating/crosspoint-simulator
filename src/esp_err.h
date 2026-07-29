#pragma once

#include <stdint.h>

typedef int esp_err_t;

#define ESP_OK 0
#define ESP_ERR_NO_MEM -1
#define ESP_FAIL -2

inline const char *esp_err_to_name(esp_err_t error) {
  switch (error) {
  case ESP_OK:
    return "ESP_OK";
  case ESP_ERR_NO_MEM:
    return "ESP_ERR_NO_MEM";
  case ESP_FAIL:
    return "ESP_FAIL";
  default:
    return "ESP_ERR_UNKNOWN";
  }
}
