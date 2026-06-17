#pragma once

#include "Arduino.h"

class WiFiClientSecure : public WiFiClient {
public:
    WiFiClientSecure() {}
    void setCACert(const char *) {}
    void setCACertBundle(const uint8_t *) {}  // no-op: native test host has no TLS stack
    void setCertificate(const char *) {}
    void setPrivateKey(const char *) {}
    void setPreSharedKey(const char *, const char *) {}
    void setInsecure() {}
    int connect(const char *host, uint16_t port) { (void)host; (void)port; return 1; }
    int connect(IPAddress ip, uint16_t port) { (void)ip; (void)port; return 1; }
};
