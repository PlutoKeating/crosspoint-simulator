#pragma once

namespace SimulatorLifecycle {

enum class WakeReason { None, PowerButton };

void initProcessArgs(char** argv);
WakeReason consumeWakeReason();
[[noreturn]] void rebootAsPowerWake();

// Host provisioning, read once per key: `Module.simConfig[key]` in the web
// build, env CROSSPOINT_SIM_CONFIG_<KEY_UPPER> natively. nullptr when absent.
const char* config(const char* key);

// Before setup(): when device_id and device_token are provisioned, write them
// into the simulated SD Project.Stick store as a bound identity so the
// unmodified firmware boots as that device instead of pairing.
void applyProvisionedIdentity();

}  // namespace SimulatorLifecycle
