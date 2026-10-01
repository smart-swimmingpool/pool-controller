// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file OtaDownloadSession.hpp
 * @brief Incremental OTA download: one bounded step per main-loop iteration.
 *
 * The firmware download used to run in a blocking loop, which paused rule
 * evaluation, sensor handling and the web portal for the whole transfer.
 * OtaUpdater now calls step() once per loop() iteration. A step never waits
 * for data and moves at most one buffer of bytes from the stream to the
 * flash sink, so the control loop keeps running during an update.
 *
 * Templated on the stream (available()/readBytes()) and the sink (write()),
 * so it is hardware-independent and covered by native unit tests.
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "OtaDownloadGuard.hpp"

namespace PoolController {

class OtaDownloadSession {
public:
  enum class Result : uint8_t {
    IN_PROGRESS,   ///< Call step() again in the next loop iteration
    COMPLETE,      ///< All bytes written
    STALLED,       ///< No data for longer than the stall timeout
    TIMED_OUT,     ///< Overall time limit exceeded
    WRITE_ERROR,   ///< Sink accepted fewer bytes than given
    DISCONNECTED,  ///< Connection closed before all bytes were received
  };

  OtaDownloadSession(uint32_t startMs, size_t totalSize, uint32_t stallTimeoutMs, uint32_t totalTimeoutMs)
      : guard_(startMs, stallTimeoutMs, totalTimeoutMs), totalSize_(totalSize) {}

  /**
   * @brief Perform one bounded download step.
   *
   * Reads at most @p bufferSize bytes that are already available and writes
   * them to @p sink. Returns immediately when no data is available.
   */
  template <typename Stream, typename Sink>
  Result step(Stream &stream, Sink &sink, bool connected, uint32_t nowMs, uint8_t *buffer, size_t bufferSize) {
    if (written_ >= totalSize_) {
      return Result::COMPLETE;
    }
    switch (guard_.check(nowMs)) {
    case OtaDownloadGuard::Status::STALLED:
      return Result::STALLED;
    case OtaDownloadGuard::Status::TIMED_OUT:
      return Result::TIMED_OUT;
    case OtaDownloadGuard::Status::OK:
      break;
    }

    size_t available = static_cast<size_t>(stream.available());
    if (available == 0) {
      return connected ? Result::IN_PROGRESS : Result::DISCONNECTED;
    }
    size_t toRead = available;
    if (toRead > bufferSize) {
      toRead = bufferSize;
    }
    if (toRead > totalSize_ - written_) {
      toRead = totalSize_ - written_;
    }
    size_t read = stream.readBytes(buffer, toRead);
    if (read == 0) {
      return Result::IN_PROGRESS;
    }
    guard_.onData(nowMs);

    if (sink.write(buffer, read) != read) {
      return Result::WRITE_ERROR;
    }
    written_ += read;
    return written_ >= totalSize_ ? Result::COMPLETE : Result::IN_PROGRESS;
  }

  size_t bytesWritten() const { return written_; }
  size_t totalSize() const { return totalSize_; }
  int progressPercent() const {
    return totalSize_ == 0 ? 0 : static_cast<int>((static_cast<uint64_t>(written_) * 100U) / totalSize_);
  }

private:
  OtaDownloadGuard guard_;
  size_t totalSize_;
  size_t written_ = 0;
};

}  // namespace PoolController
