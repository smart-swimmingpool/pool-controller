// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file Logger.hpp
 * @brief Lightweight serial logger — log levels, flag-based output control.
 */

#pragma once

#include <Arduino.h>
#include <cstdint>
#include <cstddef>

namespace PoolController {
namespace Nodes {

/**
 * Lightweight serial logger — replaces the former Node-based Logger.
 * All output goes to Serial. The struct is kept for API compatibility with
 * sites that previously used Logger::LogLevel / Logger::Flags.
 */
struct Logger final {
  Logger() = default;

  enum struct LogLevel : std::size_t { Debug = 0, Info, Warning, Critical, Error };

  struct Flags final {
    using Bits = std::uint8_t;
    enum $ : Bits { None = 0, LogToSerial = 1 << 0, LogToJson = 1 << 1, FlushLog = 1 << 2 };
  };

  LogLevel CurrentLogLevel{LogLevel::Info};
  Flags::Bits CurrentFlags{Flags::LogToSerial};

  [[nodiscard]] auto operator[](LogLevel logLevel) const noexcept -> const char * {
    return *(LOG_LEVEL_NAMES + static_cast<std::size_t>(logLevel));
  }
  [[nodiscard]] auto operator*() const noexcept -> Flags::Bits { return this->CurrentFlags; }
  [[nodiscard]] auto operator*() noexcept -> Flags::Bits & { return this->CurrentFlags; }
  [[nodiscard]] explicit operator bool() const noexcept { return (**this & Flags::LogToSerial) != 0; }
  inline auto AddFlags(const Flags::Bits x) noexcept -> Flags::Bits { return **this |= x; }
  inline auto RemoveFlags(const Flags::Bits x) noexcept -> Flags::Bits { return **this &= ~x; }
  inline auto ToggleFlags(const Flags::Bits x) noexcept -> Flags::Bits { return **this ^= x; }
  inline auto ClearFlags() noexcept -> Flags::Bits { return **this ^= **this; }

private:
  static constexpr const char *LOG_LEVEL_NAMES[]{"Debug", "Info", "Warning", "Critical", "Error"};
};

}  // namespace Nodes
}  // namespace PoolController
