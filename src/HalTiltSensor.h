#pragma once

#include <Arduino.h>

#include "HalGPIO.h"

class HalTiltSensor;
extern HalTiltSensor halTiltSensor;

// Mirrors the firmware HAL (lib/hal/HalTiltSensor.h): the IMU is only found at
// boot and kept in standby; nothing reads it.
class HalTiltSensor {
private:
  bool _available = false;

public:
  void begin() {
#if defined(SIMULATOR_DEVICE_X3)
    _available = true;
#else
    _available = false;
#endif
  }

  bool deepSleep() { return _available; }

  bool isAvailable() const { return _available; }
};
