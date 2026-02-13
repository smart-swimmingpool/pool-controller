#include "Logger.hpp"

namespace PoolController {
    namespace Nodes {
        auto Logger::mergeLevelStrings() -> String {
            String result { };
            for (auto&& name : Logger::LOG_LEVEL_NAMES) {
                result.concat(name);
                result.concat(':');
            }
            return result;
        }

        Logger::Logger() : HomieNode { "Log", "Logger", "Logger" } {
            static const String mergedString { mergeLevelStrings() };
            advertise("log").setName("log output").setDatatype("String");
            advertise("Level").settable().setName("Loglevel").setDatatype("enum").setFormat(mergedString.c_str());
        }
    }
}
