#pragma once

#include <Homie.h>

namespace PoolController
{
    extern auto setupProxy() -> void;

    struct PoolControllerContext final
    {
        PoolControllerContext();
        PoolControllerContext(const PoolControllerContext&) = delete;
        PoolControllerContext(PoolControllerContext&&) = delete;
        auto operator = (const PoolControllerContext&) -> PoolControllerContext& = delete;
        auto operator = (PoolControllerContext&&) -> PoolControllerContext& = delete;
        ~PoolControllerContext();

        auto startup() -> void;
        auto loop() -> void;

    private:
        friend auto setupProxy() -> void;

        auto setupHandler() -> void;

        HomieSetting<long>          loopIntervalSetting_ { "loop-interval", "The processing interval in seconds" };
        HomieSetting<double>        temperatureMaxPoolSetting_ { "temperature-max-pool", "Maximum temperature of solar" };
        HomieSetting<double>        temperatureMinSolarSetting_ { "temperature-min-solar", "Minimum temperature of solar" };
        HomieSetting<double>        temperatureHysteresisSetting_ { "temperature-hysteresis", "Temperature hysteresis" };
        HomieSetting<const char*>   operationModeSetting_ { "operation-mode", "Operational Mode" };
    };
}
