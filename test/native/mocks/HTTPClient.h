#pragma once

#include <cstdint>
#include <string>

#include "Arduino.h"

#define HTTPC_STRICT_FOLLOW_REDIRECTS 1

class WiFiClient;

// Test-controllable response state shared by all HTTPClient mock instances.
// Mirrors what the real ESP32 HTTPClient returns from a server response.
struct HttpCapture {
  int httpCode = 200;
  int contentSize = 0;
  std::string responseBody;
  int getStringCalls = 0;

  void clear() {
    httpCode = 200;
    contentSize = 0;
    responseBody.clear();
    getStringCalls = 0;
  }
};

extern HttpCapture httpCapture;

class HTTPClient {
public:
  HTTPClient() {}
  ~HTTPClient() {}

  bool begin(WiFiClient &client, const String &url) {
    (void)client;
    (void)url;
    return true;
  }
  bool begin(const String &url) {
    (void)url;
    return true;
  }

  int GET() { return httpCapture.httpCode; }
  int PUT() { return 200; }
  int POST() { return 200; }

  void addHeader(const String &name, const String &value) {
    (void)name;
    (void)value;
  }
  void setAuthorization(const char *type, const char *credentials) {
    (void)type;
    (void)credentials;
  }
  void setAuthorization(const char *auth) { (void)auth; }
  void setUserAgent(const String &ua) { (void)ua; }
  void setFollowRedirects(int) {}
  void setReuse(bool) {}
  void setTimeout(uint16_t) {}

  bool connected() { return httpCapture.httpCode == 200 && httpCapture.contentSize > 0; }

  int getSize() { return httpCapture.contentSize; }

  String getString() {
    httpCapture.getStringCalls++;
    return String(httpCapture.responseBody.c_str());
  }

  int writeToStream(void *) { return 0; }

  void end() {}

  int lastError() { return 0; }
  uint8_t errorToString(int) { return 0; }
  void setConnectTimeout(int) {}
  void setHeaders(const char **, int) {}
  int sendRequest(const char *, const uint8_t *, size_t) { return 200; }

  WiFiClient *getStreamPtr() { return &_stream; }
  WiFiClient &getStream() { return _stream; }

private:
  WiFiClient _stream;
};
