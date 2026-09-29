#include <Logging.h>

#include "network/FirmwareFlasher.h"
#include "network/OtaBootSwitch.h"

namespace firmware_flash {
Result flashFromSdPath(const char *, ProgressCb onProgress, void *ctx, bool,
                       BeforeSwitchCb) {
  LOG_DBG("FLASH",
          "[SIM] Firmware flashing is not supported in the native simulator");
  if (onProgress)
    onProgress(1, 1, ctx);
  return Result::WRITE_FAIL;
}

Result validateImageFile(const char *, size_t) {
  LOG_DBG(
      "FLASH",
      "[SIM] Firmware image validation is disabled in the native simulator");
  return Result::WRITE_FAIL;
}

const char *resultName(Result r) {
  switch (r) {
  case Result::OK:
    return "OK";
  case Result::OPEN_FAIL:
    return "OPEN_FAIL";
  case Result::TOO_SMALL:
    return "TOO_SMALL";
  case Result::TOO_LARGE:
    return "TOO_LARGE";
  case Result::BAD_MAGIC:
    return "BAD_MAGIC";
  case Result::BAD_SEGMENTS:
    return "BAD_SEGMENTS";
  case Result::BAD_CHECKSUM:
    return "BAD_CHECKSUM";
  case Result::BAD_SHA:
    return "BAD_SHA";
  case Result::BAD_SIZE:
    return "BAD_SIZE";
  case Result::NO_PARTITION:
    return "NO_PARTITION";
  case Result::OOM:
    return "OOM";
  case Result::READ_FAIL:
    return "READ_FAIL";
  case Result::ERASE_FAIL:
    return "ERASE_FAIL";
  case Result::WRITE_FAIL:
    return "UNSUPPORTED_IN_SIMULATOR";
  case Result::OTADATA_FAIL:
    return "OTADATA_FAIL";
  case Result::VERIFY_FAIL:
    return "VERIFY_FAIL";
  case Result::BUSY:
    return "BUSY";
  default:
    return "UNKNOWN";
  }
}
bool installInProgress() { return false; }
} // namespace firmware_flash

namespace ota_boot {
uint32_t computeSeqCrc(uint32_t) { return 0; }
bool switchTo(const esp_partition_t *) {
  LOG_DBG("FLASH", "[SIM] Boot partition switching is not supported in the "
                   "native simulator");
  return false;
}
} // namespace ota_boot

// Trial boot needs NVS and real OTA slots; the simulator never installs
// firmware, so every boot is an untracked (already trusted) image.
#include "network/OtaTrial.h"

namespace ota_trial {
void onBoot() {}
bool arm(const esp_partition_t *, const char *, const char *) {
  return false;
}
bool active() { return false; }
void noteApiResult(int, bool) {}
void tick(bool) {}
void disarm() {}
void onCleanShutdown() {}
Outcome pendingOutcome() { return {}; }
void clearOutcome() {}
} // namespace ota_trial
namespace ota_trial {
bool hasPendingOutcome() { return false; }
} // namespace ota_trial
