#pragma once

#include <Homie.h>

namespace PoolController {
    namespace Detail {
        extern auto setupProxy() -> void;
    }

    /**
     * Core controller class using RAII principles.
     * Only one instance allowed.
    */
    struct PoolControllerContext final {
        PoolControllerContext();
        PoolControllerContext(const PoolControllerContext&) = delete; // no copy
        PoolControllerContext(PoolControllerContext&&) = delete; // no move
        auto operator = (const PoolControllerContext&) -> PoolControllerContext& = delete; // no copy
        auto operator = (PoolControllerContext&&) -> PoolControllerContext& = delete; // no move
        ~PoolControllerContext();

        /**
         * Startup the controller.
         * Should be called from the standard setup() entry function.
        */
        auto setup() -> void;

        /**
         * Invoked the loop event.
         * Should be called from the standard loop() entry function.
        */
        auto loop() -> void;

    private:
        friend auto Detail::setupProxy() -> void;

        auto setupHandler() -> void;
        auto initializeController() -> void;

        HomieSetting<long>          loopIntervalSetting_ { "loop-interval", "The processing interval in seconds" };
        HomieSetting<const char*>   ntpServerSetting_ { "ntp-server", "NTP server address (e.g., pool.ntp.org, europe.pool.ntp.org)" };
        HomieSetting<long>          timezoneSetting_ { "timezone", "Timezone index (0=Central EU, 1=Eastern EU, 2=Western EU, 3=US Eastern, 4=US Central, 5=US Mountain, 6=US Pacific, 7=Australian Eastern, 8=Japan, 9=China)" };
        HomieSetting<double>        temperatureMaxPoolSetting_ { "temperature-max-pool", "Maximum temperature of solar" };
        HomieSetting<double>        temperatureMinSolarSetting_ { "temperature-min-solar", "Minimum temperature of solar" };
        HomieSetting<double>        temperatureHysteresisSetting_ { "temperature-hysteresis", "Temperature hysteresis" };
        HomieSetting<const char*>   operationModeSetting_ { "operation-mode", "Operational Mode" };
    };
}
