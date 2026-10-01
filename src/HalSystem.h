#pragma once

#include <cstdint>
#include <string>

namespace HalSystem {
struct StackFrame {
  uint32_t sp;
  uint32_t spp[8];
};

void begin();
void restart();

// Dump panic info to SD card if necessary
void checkPanic();
void clearPanic();

std::string getPanicInfo(bool full = false);
bool isRebootFromPanic();
// Heap sampling for crash reports: no-ops on the host.
void sampleHeap();
void installOutOfMemoryHandler();
} // namespace HalSystem
