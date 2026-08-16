cmake_minimum_required(VERSION 3.16)
project(pool_controller_tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ASan for buffer overflow detection
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -fsanitize=address -fno-omit-frame-pointer")

# Check whether the host libc provides strlcat (glibc ≥ 2.38, macOS, BSD).
# If not, we provide our own in stubs.cpp.
include(CheckSymbolExists)
check_symbol_exists(strlcat "string.h" HAVE_STRLCAT)
if(HAVE_STRLCAT)
  add_definitions(-DHAVE_STRLCAT)
endif()

# PlatformIO compile definitions (normally passed via -D flags)
add_definitions(-DGITHUB_REPO="smart-swimmingpool/pool-controller")
add_definitions(-DFW_VERSION="3.3.0")
add_definitions(-DNORVI_AE01_R)

# Project root
set(PROJ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# Include paths: mocks/ first so headers there shadow production headers.
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
  ${PROJ_ROOT}/src
  ${PROJ_ROOT}/lib
  ${PROJ_ROOT}/.pio/libdeps/esp32dev/ArduinoJson/src
)

# ── Wrappers ──────────────────────────────────────────────────────────────
# WebPortal, MqttPublisher, Timer, and CalibrationManager include class
# headers whose mock layout differs from production.  Compiling from
# wrappers/ avoids GCC's "current-file-directory first" rule for
# #include "..." so the mock headers take effect.
file(COPY ${PROJ_ROOT}/src/WebPortal.cpp     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/wrappers)
file(COPY ${PROJ_ROOT}/src/MqttPublisher.cpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/wrappers)
file(COPY ${PROJ_ROOT}/src/Timer.cpp         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/wrappers)
file(COPY ${PROJ_ROOT}/src/CalibrationManager.cpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/wrappers)
file(COPY ${PROJ_ROOT}/src/NorviButtonHandler.cpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/wrappers)

set(WRAPPER_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/wrappers/WebPortal.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/wrappers/MqttPublisher.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/wrappers/Timer.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/wrappers/CalibrationManager.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/wrappers/NorviButtonHandler.cpp
)

# ── Service classes ───────────────────────────────────────────────────────
# Compiled from src/ directly (their own headers are in src/, so they find
# the production header first — only Arduino-level mocks are needed).
set(SERVICE_SOURCES
  ${PROJ_ROOT}/src/StateManager.cpp
  ${PROJ_ROOT}/src/DegradationManager.cpp
  ${PROJ_ROOT}/src/SystemMonitor.cpp
  ${PROJ_ROOT}/src/OtaUpdater.cpp
  ${PROJ_ROOT}/src/LogCapture.cpp
  ${PROJ_ROOT}/src/LocalSettingsMenu.cpp
${PROJ_ROOT}/src/Ky040Decoder.cpp
  ${PROJ_ROOT}/src/OlimexEncoderHandler.cpp
)

# Mock sources (compiled once)
set(MOCK_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks/captures.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks/NetworkManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks/ConfigManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks/globals.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks/stubs.cpp
)

# Test sources
set(TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_rules.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_config_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_webportal_json.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mqttpublisher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_security.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_state_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_timer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_logcapture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_webportal_logs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_local_settings_menu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_ky040_decoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_calibration_manager.cpp
)

add_executable(test_runner
  ${MOCK_SOURCES}
  ${WRAPPER_SOURCES}
  ${SERVICE_SOURCES}
  ${TEST_SOURCES}
)

target_include_directories(test_runner PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
  ${PROJ_ROOT}/src
  ${CMAKE_CURRENT_BINARY_DIR}/wrappers
)

# Link gcov
target_link_libraries(test_runner gcov)

# Test command
add_test(NAME pool_controller_test COMMAND test_runner)
