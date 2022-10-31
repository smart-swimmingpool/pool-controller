#pragma once

namespace PoolController
{
    struct PoolControllerContext final
    {
        PoolControllerContext() = default;
        PoolControllerContext(const PoolControllerContext&) = delete;
        PoolControllerContext(PoolControllerContext&&) = delete;
        auto operator = (const PoolControllerContext&) -> PoolControllerContext& = delete;
        auto operator = (PoolControllerContext&&) -> PoolControllerContext& = delete;
        ~PoolControllerContext() = default;

        auto startup() -> void;
        auto loop() -> void;
    };
}
