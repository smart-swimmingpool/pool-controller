#pragma once

#include <Homie.h>

namespace PoolController
{
    namespace Detail
    {
        extern auto setupProxy() -> void;
    }

    struct PoolControllerContext final
    {
        PoolControllerContext();
        PoolControllerContext(const PoolControllerContext&) = delete; // no copy
        PoolControllerContext(PoolControllerContext&&) = delete; // no move
        auto operator = (const PoolControllerContext&) -> PoolControllerContext& = delete; // no copy
        auto operator = (PoolControllerContext&&) -> PoolControllerContext& = delete; // no move
        ~PoolControllerContext();

        auto startup() -> void;
        auto loop() -> void;

    private:
        friend auto Detail::setupProxy() -> void;

        auto setupHandler() -> void;

        HomieSetting<long>          loopIntervalSetting_ { "loop-interval", "The processing interval in seconds" };
        HomieSetting<double>        temperatureMaxPoolSetting_ { "temperature-max-pool", "Maximum temperature of solar" };
        HomieSetting<double>        temperatureMinSolarSetting_ { "temperature-min-solar", "Minimum temperature of solar" };
        HomieSetting<double>        temperatureHysteresisSetting_ { "temperature-hysteresis", "Temperature hysteresis" };
        HomieSetting<const char*>   operationModeSetting_ { "operation-mode", "Operational Mode" };
    };
}
