#pragma once

namespace SimulatorLifecycle {

enum class WakeReason { None, PowerButton };

void initProcessArgs(char** argv);
WakeReason consumeWakeReason();
[[noreturn]] void rebootAsPowerWake();

// Host provisioning, read once per key: `Module.simConfig[key]` in the web
// build, env CROSSPOINT_SIM_CONFIG_<KEY_UPPER> natively. nullptr when absent.
// Cached; large values (the program) are read with readConfigOnce instead.
const char* config(const char* key);

// Before setup(), from the host provisioning:
// - `device_id`: written into the simulated SD StockStick store as a bound
//   identity without a device token, so the firmware shows content instead of
//   the BLE setup QR and makes no cloud requests.
// - `program` (base64 SSP1; natively also `program_path`, a raw file): written
//   to /.crosspoint/studio/import.ssp, which a SIMULATOR firmware build
//   installs at startup through the same path BLE delivery uses.
void applyProvisioning();

}  // namespace SimulatorLifecycle
