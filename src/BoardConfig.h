#pragma once

// Keep the small portion of the FreeInk BoardConfig surface used outside the
// device HAL available without pulling ESP32-only GPIO headers into the native
// build. Device selection is compile-time in the simulator, matching the
// firmware's single-board X4 Pro build and dual X3/X4 profiles closely enough
// for capability-gated UI and network status paths.
#define FREEINK_LOG_TRANSPORT_HWCDC 0
#define FREEINK_LOG_TRANSPORT_ROM_PRINTF 1
#define FREEINK_LOG_TRANSPORT FREEINK_LOG_TRANSPORT_HWCDC

#if defined(SIMULATOR_DEVICE_X4_PRO)
#define FREEINK_DEVICE_X4 0
#define FREEINK_DEVICE_X3 0
#define FREEINK_DEVICE_X4PRO 1
#define FREEINK_CAP_TOUCH 1
#define FREEINK_CAP_FRONTLIGHT 1
#elif defined(SIMULATOR_DEVICE_X3)
#define FREEINK_DEVICE_X4 0
#define FREEINK_DEVICE_X3 1
#define FREEINK_DEVICE_X4PRO 0
#define FREEINK_CAP_TOUCH 0
#define FREEINK_CAP_FRONTLIGHT 0
#else
#define FREEINK_DEVICE_X4 1
#define FREEINK_DEVICE_X3 0
#define FREEINK_DEVICE_X4PRO 0
#define FREEINK_CAP_TOUCH 0
#define FREEINK_CAP_FRONTLIGHT 0
#endif

namespace BoardConfig {

enum class Board {
  XteinkX4,
  XteinkX3,
  XteinkX4Pro,
};

struct BoardProfile {
  Board board;
  const char *name;
};

inline constexpr BoardProfile XTEINK_X4 = {Board::XteinkX4, "xteink_x4"};
inline constexpr BoardProfile XTEINK_X3 = {Board::XteinkX3, "xteink_x3"};
inline constexpr BoardProfile XTEINK_X4_PRO = {Board::XteinkX4Pro,
                                               "xteink_x4_pro"};

#if defined(SIMULATOR_DEVICE_X4_PRO)
inline BoardProfile ACTIVE = XTEINK_X4_PRO;
#elif defined(SIMULATOR_DEVICE_X3)
inline BoardProfile ACTIVE = XTEINK_X3;
#else
inline BoardProfile ACTIVE = XTEINK_X4;
#endif

inline bool selectDevice(Board board) {
  switch (board) {
  case Board::XteinkX4:
    ACTIVE = XTEINK_X4;
    return true;
  case Board::XteinkX3:
    ACTIVE = XTEINK_X3;
    return true;
  case Board::XteinkX4Pro:
    ACTIVE = XTEINK_X4_PRO;
    return true;
  }
  return false;
}

inline bool isX4Pro() { return ACTIVE.board == Board::XteinkX4Pro; }
inline bool hasTouch() { return isX4Pro(); }
inline bool hasHomeKey() { return isX4Pro(); }
inline bool hasPwmFrontlight() { return isX4Pro(); }

inline void holdPowerRails() {}

} // namespace BoardConfig
