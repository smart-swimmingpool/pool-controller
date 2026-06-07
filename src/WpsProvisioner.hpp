// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file WpsProvisioner.hpp
 * @brief WiFi WPS push-button provisioning utility.
 */

#pragma once

namespace PoolController {

/**
 * @brief Static utility that provides WPS push-button WiFi provisioning.
 *
 * On boot, if the BOOT button (GPIO0) is held for >2 seconds, WPS Push
 * Button Connect (PBC) is initiated. If pairing succeeds, the WiFi
 * credentials are persisted to ConfigManager/LittleFS.
 */
class WpsProvisioner final {
public:
  WpsProvisioner() = delete;
  WpsProvisioner(const WpsProvisioner &) = delete;
  WpsProvisioner(WpsProvisioner &&) = delete;
  auto operator=(const WpsProvisioner &) -> WpsProvisioner & = delete;
  auto operator=(WpsProvisioner &&) -> WpsProvisioner & = delete;

  /**
   * @brief Check the BOOT button and start WPS provisioning if triggered.
   * Call once during setup, before WiFi connection attempts.
   */
  static auto runIfRequested() -> void;
};

}  // namespace PoolController
