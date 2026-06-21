#pragma once
#include <stdint.h>
#include "Arduino.h"

enum class DNSReplyCode : uint8_t {
  NoError = 0,
  FormError = 1,
  ServerFailure = 2,
  NXDomain = 3,
  NotImplemented = 4,
  Refused = 5
};

class DNSServer {
public:
  void start(uint16_t port, const char *domain, const char *ip) {}
  void start(uint16_t port, const char *domain, IPAddress ip) {}
  void stop() {}
  void processNextRequest() {}
  void setErrorReplyCode(DNSReplyCode code) {}
};
