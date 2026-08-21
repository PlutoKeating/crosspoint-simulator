#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "HTTPClient.h"
#include "NetworkClient.h"
#include "WString.h"

namespace freeink {

class SecureHttpClient {
public:
  using DataCallback = std::function<bool(const uint8_t *, size_t)>;
  void setCACert(const char *) {}
  void setInsecure() {}

  bool begin(const String &url) {
    http_.begin(client_, url.c_str());
    return !url.isEmpty();
  }

  bool begin(const std::string &url) { return begin(String(url)); }
  bool begin(const char *url) { return begin(String(url)); }

  void end() { http_.end(); }

  void addHeader(const char *name, const String &value) {
    http_.addHeader(name, value);
  }

  void addHeader(const char *name, const std::string &value) {
    http_.addHeader(name, value.c_str());
  }

  void addHeader(const char *name, const char *value) {
    http_.addHeader(name, value);
  }

  void setTimeout(uint16_t ms) { http_.setTimeout(ms); }
  void setReuse(bool reuse) { http_.setReuse(reuse); }

  int GET() {
    status_ = http_.GET();
    responseComplete_ = status_ > 0;
    return status_;
  }
  int GET(const DataCallback &onData) {
    status_ = http_.GET();
    responseComplete_ = status_ > 0;
    callbackAborted_ = false;
    if (status_ == 200 && onData) {
      const String body = http_.getString();
      callbackAborted_ = !onData(reinterpret_cast<const uint8_t *>(body.c_str()), body.length());
    }
    return status_;
  }
  int POST(const String &payload) { return http_.POST(payload.c_str()); }
  int sendRequest(const char *method, const String &payload) {
    if (method && std::string(method) == "PUT") {
      return http_.PUT(payload);
    }
    if (method && std::string(method) == "POST") {
      return http_.POST(payload.c_str());
    }
    return http_.GET();
  }
  int sendRequest(const char *method, const std::string &payload) {
    return sendRequest(method, String(payload));
  }

  String getString() { return http_.getString(); }
  int getSize() { return http_.getSize(); }
  int getStatus() const { return status_; }
  bool responseComplete() const { return responseComplete_; }
  bool callbackAborted() const { return callbackAborted_; }

  static bool tls13Available() { return true; }

private:
  NetworkClientSecure client_;
  HTTPClient http_;
  int status_ = 0;
  bool responseComplete_ = false;
  bool callbackAborted_ = false;
};

} // namespace freeink
