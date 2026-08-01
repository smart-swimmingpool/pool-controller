// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file LogCapture.cpp
 * @brief Central ring-buffer logger implementation.
 *
 * Static RAM ring buffer (no heap), optionally mirrored to Serial.
 * On ESP32 the ring is guarded by a portMUX critical section so it is safe
 * from loop, WebServer handlers, and WiFi/MQTT callbacks. Native tests
 * (no ESP32 macros) compile the guard to a no-op.
 */

#include "LogCapture.hpp"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>

// The mock Arduino.h defines ARDUINO but no ESP32 macro, so the guard keys
// off the ESP32 macros like the rest of the codebase (OtaUpdater.cpp:537).
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
static portMUX_TYPE s_logMux = portMUX_INITIALIZER_UNLOCKED;
#define LOG_CRITICAL_ENTER() portENTER_CRITICAL(&s_logMux)
#define LOG_CRITICAL_EXIT() portEXIT_CRITICAL(&s_logMux)
#else
#define LOG_CRITICAL_ENTER() ((void)0)
#define LOG_CRITICAL_EXIT() ((void)0)
#endif

namespace PoolController {

// Static storage — fixed size, no heap.
LogEntry LogCapture::s_buffer[LogCapture::LOG_BUFFER_ENTRIES];
std::size_t LogCapture::s_head = 0;
std::uint32_t LogCapture::s_seq = 0;
std::uint32_t LogCapture::s_clearedSeq = 0;
bool LogCapture::s_logToSerial = true;

void LogCapture::begin() {
  LOG_CRITICAL_ENTER();
  s_head = 0;
  s_seq = 0;
  s_clearedSeq = 0;
  s_logToSerial = true;
  LOG_CRITICAL_EXIT();
}

void LogCapture::log(LogLevel level, const char *fmt, ...) {
  if (fmt == nullptr) {
    return;
  }
  // Format into the full-size buffer so the Serial mirror (in store())
  // receives the complete message; only the ring copy is truncated.
  char message[LOG_FORMAT_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  message[LOG_FORMAT_SIZE - 1] = '\0';
  store(level, message);
}

void LogCapture::logEvent(const char *eventType, const char *fmt, ...) {
  if (eventType == nullptr || fmt == nullptr) {
    return;
  }
  char body[LOG_FORMAT_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(body, sizeof(body), fmt, args);
  va_end(args);
  body[LOG_FORMAT_SIZE - 1] = '\0';

  char message[LOG_FORMAT_SIZE + 64];  // room for the "[TYPE] " prefix
  snprintf(message, sizeof(message), "[%s] %s", eventType, body);
  store(LogLevel::Info, message);
}

void LogCapture::store(LogLevel level, const char *message) {
  const bool mirror = s_logToSerial;

  LOG_CRITICAL_ENTER();
  ++s_seq;
  LogEntry &entry = s_buffer[s_head];
  entry.seq = s_seq;
  entry.uptimeMs = millis();
  entry.level = level;
  strncpy(entry.message, message, LOG_MSG_SIZE - 1);
  entry.message[LOG_MSG_SIZE - 1] = '\0';
  s_head = (s_head + 1) % LOG_BUFFER_ENTRIES;
  LOG_CRITICAL_EXIT();

  // Mirror after leaving the critical section — Serial.print can block.
  if (mirror) {
    Serial.print(message);
  }
}

std::size_t LogCapture::getEntries(
  std::uint32_t sinceSeq, std::size_t maxCount, LogLevel minLevel, LogEntry *out, std::size_t outCapacity) {
  const std::size_t cap = (maxCount < outCapacity) ? maxCount : outCapacity;
  if (out == nullptr || cap == 0) {
    return 0;
  }

  std::size_t written = 0;
  LOG_CRITICAL_ENTER();
  // Entries written since the last clear (watermark hides pre-clear entries).
  const std::uint32_t postClear = s_seq - s_clearedSeq;
  const std::size_t visible = (postClear < LOG_BUFFER_ENTRIES) ? static_cast<std::size_t>(postClear) : LOG_BUFFER_ENTRIES;
  // After clear() the ring restarts at slot 0; once wrapped, s_head is oldest.
  const std::size_t oldest = (postClear < LOG_BUFFER_ENTRIES) ? 0 : s_head;

  for (std::size_t i = 0; i < visible && written < cap; ++i) {
    const LogEntry &entry = s_buffer[(oldest + i) % LOG_BUFFER_ENTRIES];
    if (entry.seq <= sinceSeq) {
      continue;
    }
    if (entry.level < minLevel) {
      continue;
    }
    out[written++] = entry;
  }
  LOG_CRITICAL_EXIT();
  return written;
}

std::uint32_t LogCapture::lastSeq() {
  LOG_CRITICAL_ENTER();
  const std::uint32_t seq = s_seq;
  LOG_CRITICAL_EXIT();
  return seq;
}

void LogCapture::clear() {
  LOG_CRITICAL_ENTER();
  // Hide everything written so far. s_seq is NOT reset so polling clients
  // (which track seq) keep working; new entries continue above the watermark.
  s_clearedSeq = s_seq;
  s_head = 0;
  LOG_CRITICAL_EXIT();
}

const char *LogCapture::levelName(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return "debug";
  case LogLevel::Info:
    return "info";
  case LogLevel::Warning:
    return "warning";
  case LogLevel::Critical:
    return "critical";
  case LogLevel::Error:
    return "error";
  }
  return "info";
}

LogLevel LogCapture::parseLevel(const char *name) {
  if (name == nullptr) {
    return LogLevel::Info;
  }
  // Case-insensitive compare without relying on platform string.h.
  auto iequals = [](const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
      if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) {
        return false;
      }
      ++a;
      ++b;
    }
    return *a == *b;
  };

  if (iequals(name, "debug")) {
    return LogLevel::Debug;
  }
  if (iequals(name, "warning")) {
    return LogLevel::Warning;
  }
  if (iequals(name, "error")) {
    return LogLevel::Error;
  }
  if (iequals(name, "critical")) {
    return LogLevel::Critical;
  }
  return LogLevel::Info;
}

bool LogCapture::isLogToSerial() {
  return s_logToSerial;
}

void LogCapture::setLogToSerial(bool enabled) {
  s_logToSerial = enabled;
}

}  // namespace PoolController
