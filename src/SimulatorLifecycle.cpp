#include "SimulatorLifecycle.h"

#include <ArduinoJson.h>
#include <sys/stat.h>

#include <cctype>
#include <cstdint>
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

namespace {

// Uncached read of a (possibly megabyte-sized) provisioning value.
std::string readConfigOnce(const char *key) {
  std::string value;
#ifdef __EMSCRIPTEN__
  const int length = MAIN_THREAD_EM_ASM_INT(
      {
        var c = Module.simConfig;
        var v = c && c[UTF8ToString($0)];
        return typeof v === 'string' ? lengthBytesUTF8(v) : -1;
      },
      key);
  if (length > 0) {
    value.resize(static_cast<size_t>(length) + 1);
    MAIN_THREAD_EM_ASM(
        { stringToUTF8(Module.simConfig[UTF8ToString($0)], $1, $2); }, key,
        &value[0], length + 1);
    value.resize(static_cast<size_t>(length));
  }
#else
  if (const char *v = config(key))
    value = v;
#endif
  return value;
}

bool decodeBase64(const std::string &text, std::string &out) {
  auto sextet = [](char c) -> int {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+' || c == '-') return 62;
    if (c == '/' || c == '_') return 63;
    return -1;
  };
  out.clear();
  out.reserve(text.size() / 4 * 3);
  uint32_t buffer = 0;
  int bits = 0;
  for (char c : text) {
    if (c == '=' || c == '\n' || c == '\r') continue;
    const int v = sextet(c);
    if (v < 0) return false;
    buffer = (buffer << 6) | static_cast<uint32_t>(v);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out.push_back(static_cast<char>((buffer >> bits) & 0xff));
    }
  }
  return true;
}

std::string sdRoot() {
  const char *rootEnv = std::getenv("CROSSPOINT_SIM_SD");
  if (!rootEnv || !*rootEnv)
    rootEnv = std::getenv("CROSSPOINT_EMU_SD");
  std::string root = (rootEnv && *rootEnv) ? rootEnv : "./fs_";
  while (root.size() > 1 && root.back() == '/')
    root.pop_back();
  return root;
}

void writeAtomically(const std::string &path, const std::string &bytes) {
  const std::string temp = path + ".tmp";
  {
    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  }
  std::rename(temp.c_str(), path.c_str());
}

void provisionIdentity(const std::string &dir, const char *id) {
  const std::string path = dir + "/project_stick.json";
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
  doc["bound"] = true;
  // No credential: the demo device never calls the device API.
  doc.remove("device_token");
  std::string json;
  serializeJson(doc, json);
  writeAtomically(path, json);
}

void provisionProgram(const std::string &dir) {
  std::string bytes;
  const std::string encoded = readConfigOnce("program");
  if (!encoded.empty()) {
    if (!decodeBase64(encoded, bytes)) {
      std::fputs("SimulatorLifecycle: invalid base64 program\n", stderr);
      return;
    }
  } else if (const char *file = config("program_path")) {
    std::ifstream in(file, std::ios::binary);
    if (!in)
      return;
    std::stringstream text;
    text << in.rdbuf();
    bytes = text.str();
  }
  if (bytes.size() < 8 || bytes.compare(0, 4, "SSP1") != 0)
    return;
  const std::string studio = dir + "/studio";
  ::mkdir(studio.c_str(), 0777);
  writeAtomically(studio + "/import.ssp", bytes);
}

} // namespace

void applyProvisioning() {
  const std::string root = sdRoot();
  const std::string dir = root + "/.crosspoint";
  ::mkdir(root.c_str(), 0777);
  ::mkdir(dir.c_str(), 0777);
  if (const char *id = config("device_id"))
    provisionIdentity(dir, id);
  provisionProgram(dir);
}

} // namespace SimulatorLifecycle
