// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

namespace PoolController {

/**
 * Core controller class using RAII principles.
 * Only one instance allowed.
 */
struct PoolControllerContext final {
  PoolControllerContext();
  // no copy
  PoolControllerContext(const PoolControllerContext &) = delete;
  // no move
  PoolControllerContext(PoolControllerContext &&) = delete;
  // no copy
  auto operator=(const PoolControllerContext &) -> PoolControllerContext & = delete;
  // no move
  auto operator=(PoolControllerContext &&) -> PoolControllerContext & = delete;
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
  auto initializeController() -> void;

  bool bootLoopDetected_ = false;
};

}  // namespace PoolController
