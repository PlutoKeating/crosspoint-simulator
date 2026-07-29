#pragma once
#include "esp_err.h"

inline void esp_task_wdt_reset() {}
inline void esp_task_wdt_init(int, bool) {}
inline void esp_task_wdt_add(void *) {}
inline esp_err_t esp_task_wdt_status(void *) { return ESP_OK; }
