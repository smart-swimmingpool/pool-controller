#pragma once

#include <stdint.h>
#include <string>
#include "Arduino.h"

#define HTTPC_STRICT_FOLLOW_REDIRECTS 1

class WiFiClient;

class HTTPClient {
public:
    HTTPClient() {}
    ~HTTPClient() {}

    bool begin(WiFiClient &client, const String &url) { (void)client; (void)url; return true; }
    bool begin(const String &url) { (void)url; return true; }
    int GET() { return 200; }
    int PUT() { return 200; }
    int POST() { return 200; }
    void addHeader(const String &name, const String &value) { (void)name; (void)value; }
    void setAuthorization(const char *type, const char *credentials) { (void)type; (void)credentials; }
    void setAuthorization(const char *auth) { (void)auth; }
    void setUserAgent(const String &ua) { (void)ua; }
    void setFollowRedirects(int) {}
    void setReuse(bool) {}
    void setTimeout(uint16_t) {}
    bool connected() { return false; }
    int getSize() { return 0; }
    String getString() { return String("{}"); }
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
