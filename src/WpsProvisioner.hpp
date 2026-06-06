// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

namespace PoolController {
class WpsProvisioner final {
public:
  WpsProvisioner() = delete;
  WpsProvisioner(const WpsProvisioner &) = delete;
  WpsProvisioner(WpsProvisioner &&) = delete;
  auto operator=(const WpsProvisioner &) -> WpsProvisioner & = delete;
  auto operator=(WpsProvisioner &&) -> WpsProvisioner & = delete;

  static auto runIfRequested() -> bool;
};
}  // namespace PoolController
