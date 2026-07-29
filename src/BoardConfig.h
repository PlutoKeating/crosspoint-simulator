#pragma once

// Minimal native counterpart of freeink-sdk's BoardConfig facade. X3/X4 have
// no touch panel, and simulator startup does not need physical power-rail GPIO.

#define FREEINK_DEVICE_X3 1
#define FREEINK_DEVICE_X4 0
#define FREEINK_LOG_TRANSPORT_SERIAL 0
#define FREEINK_LOG_TRANSPORT_USB_CDC_WRITE 1
#define FREEINK_LOG_TRANSPORT_ROM_PRINTF 2
#define FREEINK_LOG_TRANSPORT FREEINK_LOG_TRANSPORT_SERIAL

namespace BoardConfig {

inline void holdPowerRails() {}
inline bool hasTouch() { return false; }

} // namespace BoardConfig
