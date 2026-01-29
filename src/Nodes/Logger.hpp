#pragma once

#include "Homie.hpp"
#include "HomieNode.hpp"

namespace PoolController {
    namespace Nodes {
        struct Logger final : HomieNode {
            Logger();

            enum struct LogLevel : std::size_t {
                Debug = 0,
                Info,
                Warning,
                Critical,
                Error
            };

            struct Flags final {
                using Bits = std::uint8_t;
                enum $ : Bits {
                    None = 0,
                    LogToSerial = 1 << 0,
                    LogToJson = 1 << 1,
                    FlushLog = 1 << 2
                };
            };

            LogLevel CurrentLogLevel { LogLevel::Info };
            Flags::Bits CurrentFlags { Flags::LogToSerial | Flags::LogToJson };
            [[nodiscard]] auto operator [] (LogLevel logLevel) const noexcept -> const char* { return *(LOG_LEVEL_NAMES + static_cast<std::size_t>(logLevel)); }
            [[nodiscard]] auto operator *() const noexcept -> Flags::Bits { return this->CurrentFlags; }
            [[nodiscard]] auto operator *() noexcept -> Flags::Bits& { return this->CurrentFlags; }
            [[nodiscard]] explicit operator bool() const noexcept { return Homie.isConnected() || **this & Flags::LogToSerial; }
            inline auto AddFlags(const Flags::Bits x) noexcept -> Flags::Bits { return **this |= x; }
            inline auto RemoveFlags(const Flags::Bits x) noexcept -> Flags::Bits { return **this &= ~x; }
            inline auto ToggleFlags(const Flags::Bits x) noexcept -> Flags::Bits { return **this ^= x; }
            inline auto ClearFlags() noexcept -> Flags::Bits { return **this ^= **this; }

        private:
            static constexpr const char* LOG_LEVEL_NAMES[]
            {
                "Debug",
                "Info",
                "Warning",
                "Critical",
                "Error"
            };
            [[nodiscard]] static auto mergeLevelStrings() -> String;
            virtual auto setup() -> void override;
            virtual auto onReadyToOperate() -> void override;
            [[nodiscard]] virtual auto handleInput(const HomieRange& range, const String& property, const String& value) -> bool override;
        };
    }
}
