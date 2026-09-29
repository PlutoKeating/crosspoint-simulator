#include "SimulatorLifecycle.h"

#include <ArduinoJson.h>
#include <sys/stat.h>

#include <cctype>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace {

constexpr const char *kWakeReasonEnv = "CROSSPOINT_SIM_WAKE_REASON";
constexpr const char *kInputScriptEnv = "CROSSPOINT_SIM_INPUT_SCRIPT";
constexpr const char *kInputScriptAfterWakeEnv =
    "CROSSPOINT_SIM_INPUT_SCRIPT_AFTER_WAKE";
constexpr const char *kScreenshotsEnv = "CROSSPOINT_SIM_SCREENSHOTS";
constexpr const char *kScreenshotsAfterWakeEnv =
    "CROSSPOINT_SIM_SCREENSHOTS_AFTER_WAKE";
char **gArgv = nullptr;

void promoteAfterWakeValue(const char *target, const char *afterWake) {
  const char *value = std::getenv(afterWake);
  if (value) {
    setenv(target, value, 1);
  } else {
    unsetenv(target);
  }
  unsetenv(afterWake);
}

} // namespace

namespace SimulatorLifecycle {

void initProcessArgs(char **argv) { gArgv = argv; }

WakeReason consumeWakeReason() {
  const char *value = std::getenv(kWakeReasonEnv);
  if (!value) {
    return WakeReason::None;
  }

  unsetenv(kWakeReasonEnv);
  if (std::strcmp(value, "power") == 0) {
    return WakeReason::PowerButton;
  }
  return WakeReason::None;
}

[[noreturn]] void rebootAsPowerWake() {
#ifdef __EMSCRIPTEN__
  // A browser tab cannot re-exec itself: the page persists the SD image and
  // reloads with a power-button wake reason (see the web loader).
  MAIN_THREAD_EM_ASM({
    if (Module.simReboot) Module.simReboot("power");
  });
  while (true)
    usleep(1000 * 1000);
#else
  if (!gArgv || !gArgv[0]) {
    std::fputs("SimulatorLifecycle: missing argv for reboot\n", stderr);
    _exit(1);
  }

  setenv(kWakeReasonEnv, "power", 1);
  // A deep-sleep wake is a fresh process. Do not replay the pre-sleep script,
  // which would otherwise put every relaunched process back to sleep forever.
  // Tests can provide an explicit post-wake schedule when they need to capture
  // or terminate the relaunched instance.
  promoteAfterWakeValue(kInputScriptEnv, kInputScriptAfterWakeEnv);
  promoteAfterWakeValue(kScreenshotsEnv, kScreenshotsAfterWakeEnv);
  execvp(gArgv[0], gArgv);

  std::perror("execvp");
  _exit(1);
#endif
}

const char *config(const char *key) {
  static std::mutex mutex;
  static std::map<std::string, std::string> cache;
  std::lock_guard<std::mutex> lock(mutex);
  auto hit = cache.find(key);
  if (hit != cache.end())
    return hit->second.empty() ? nullptr : hit->second.c_str();
  std::string value;
#ifdef __EMSCRIPTEN__
  // Module lives on the page's main thread; firmware code runs on a worker.
  const int length = MAIN_THREAD_EM_ASM_INT(
      {
        var c = Module.simConfig;
        var v = c && c[UTF8ToString($0)];
        return typeof v === 'string' ? lengthBytesUTF8(v) : -1;
      },
      key);
  if (length > 0) {
    std::vector<char> buffer(static_cast<size_t>(length) + 1, '\0');
    MAIN_THREAD_EM_ASM(
        { stringToUTF8(Module.simConfig[UTF8ToString($0)], $1, $2); }, key,
        buffer.data(), length + 1);
    value.assign(buffer.data(), static_cast<size_t>(length));
  }
#else
  std::string name = "CROSSPOINT_SIM_CONFIG_";
  for (const char *p = key; *p; ++p)
    name += static_cast<char>(
        std::toupper(static_cast<unsigned char>(*p)));
  if (const char *env = std::getenv(name.c_str()))
    value = env;
#endif
  auto inserted = cache.emplace(key, value).first;
  return inserted->second.empty() ? nullptr : inserted->second.c_str();
}

void applyProvisionedIdentity() {
  const char *id = config("device_id");
  const char *token = config("device_token");
  if (!id || !token)
    return;
  const char *rootEnv = std::getenv("CROSSPOINT_SIM_SD");
  if (!rootEnv || !*rootEnv)
    rootEnv = std::getenv("CROSSPOINT_EMU_SD");
  std::string root = (rootEnv && *rootEnv) ? rootEnv : "./fs_";
  while (root.size() > 1 && root.back() == '/')
    root.pop_back();
  const std::string dir = root + "/.crosspoint";
  const std::string path = dir + "/project_stick.json";
  ::mkdir(root.c_str(), 0777);
  ::mkdir(dir.c_str(), 0777);

  JsonDocument doc;
  {
    std::ifstream in(path);
    if (in) {
      std::stringstream text;
      text << in.rdbuf();
      if (deserializeJson(doc, text.str()) || !doc.is<JsonObject>())
        doc.clear();
    }
  }
  const std::string previous = doc["device_id"] | "";
  if (previous != id) {
    // Another identity's binding state must not leak into the provisioned one.
    doc.remove("owner_id");
    doc.remove("pending_events");
  }
  doc["device_id"] = id;
  doc["device_token"] = token;
  doc["bound"] = true;
  doc.remove("pairing_code");
  const std::string temp = path + ".tmp";
  {
    std::ofstream out(temp, std::ios::trunc);
    serializeJson(doc, out);
  }
  std::rename(temp.c_str(), path.c_str());
}

} // namespace SimulatorLifecycle
