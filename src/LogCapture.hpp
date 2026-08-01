// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file LogCapture.hpp
 * @brief Central ring-buffer logger — replaces direct Serial.* calls.
 *
 * All firmware logging flows through LogCapture: entries are stored in a
 * fixed-size RAM ring buffer (no heap) and optionally mirrored to Serial
 * (byte-identical, preserving existing serial debugging). REST and MQTT
 * consumers read from the same buffer.
 *
 * Thread safety: on ESP32 the buffer is guarded by a portMUX critical
 * section (callable from loop, WebServer handlers, and WiFi/MQTT callbacks).
 * Native tests (no ESP32 macros) compile the guard to a no-op.
 */

#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>

// Configurable via PlatformIO build_flags (-DLOG_BUFFER_SIZE=...).
#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 8192
#endif

#ifndef LOG_MSG_SIZE
#define LOG_MSG_SIZE 96
#endif

namespace PoolController {

/**
 * Log levels, ordered by severity (higher = more important).
 */
enum class LogLevel : std::uint8_t { Debug = 0, Info, Warning, Critical, Error };

/**
 * A single captured log entry. Fixed size — no heap.
 */
struct LogEntry {
  std::uint32_t seq;           //!< monotonically increasing, used for since-polling
  std::uint32_t uptimeMs;      //!< millis() at capture time
  LogLevel level;              //!< severity
  char message[LOG_MSG_SIZE];  //!< formatted message (null-terminated)
};

/**
 * Central logging service with a static RAM ring buffer.
 */
class LogCapture final {
public:
  static constexpr std::size_t LOG_BUFFER_ENTRIES = LOG_BUFFER_SIZE / LOG_MSG_SIZE;

  /** Resets the ring and state. Called once at boot after Serial.begin(). */
  static void begin();
  /** Formats and stores an entry; mirrors to Serial when enabled. */
  static void log(LogLevel level, const char *fmt, ...);
  /** Logs a curated event (Info level) with a "[TYPE] message" marker for MQTT export. */
  static void logEvent(const char *eventType, const char *fmt, ...);
  /**
   * Copies up to min(maxCount, outCapacity) entries newer than sinceSeq with
   * level >= minLevel into out. Returns the number of entries written.
   */
  static std::size_t getEntries(
    std::uint32_t sinceSeq, std::size_t maxCount, LogLevel minLevel, LogEntry *out, std::size_t outCapacity);
  /** Sequence number of the last assigned entry (0 if none yet). */
  static std::uint32_t lastSeq();
  /** Empties the ring. s_seq is NOT reset so polling clients keep working. */
  static void clear();
  static const char *levelName(LogLevel level);
  /** Parses "debug"|"info"|"warning"|"error" (case-insensitive) — unknown → Info. */
  static LogLevel parseLevel(const char *name);
  static bool isLogToSerial();
  static void setLogToSerial(bool enabled);

private:
  /** Writes an already-formatted message into the ring (guarded) and mirrors to Serial. */
  static void store(LogLevel level, const char *message);

  static LogEntry s_buffer[LOG_BUFFER_ENTRIES];
  static std::size_t s_head;          //!< next free slot
  static std::uint32_t s_seq;         //!< last assigned sequence number
  static std::uint32_t s_clearedSeq;  //!< watermark: entries <= this are hidden after clear()
  static bool s_logToSerial;
};

}  // namespace PoolController

#define LOG_DEBUG(...) PoolController::LogCapture::log(PoolController::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) PoolController::LogCapture::log(PoolController::LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...) PoolController::LogCapture::log(PoolController::LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) PoolController::LogCapture::log(PoolController::LogLevel::Error, __VA_ARGS__)
