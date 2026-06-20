cmake_minimum_required(VERSION 3.16)
project(pool_controller_tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ASan for buffer overflow detection
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -fsanitize=address -fno-omit-frame-pointer")

# PlatformIO compile definitions (normally passed via -D flags)
add_definitions(-DGITHUB_REPO="smart-swimmingpool/pool-controller")
add_definitions(-DFW_VERSION="3.3.0")

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
# WebPortal and MqttPublisher include node-class headers
# (DallasTemperatureNode, RelayModuleNode, …) whose mock layout differs from
# production.  Compiling from wrappers/ avoids GCC's "current-file-directory
# first" rule for #include "..." so the mock headers take effect.
file(COPY ${PROJ_ROOT}/src/WebPortal.cpp    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/wrappers)
file(COPY ${PROJ_ROOT}/src/MqttPublisher.cpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/wrappers)

set(WRAPPER_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/wrappers/WebPortal.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/wrappers/MqttPublisher.cpp
)

# Service classes (all static methods, no instance-layout concerns) compile
# from src/ directly.
set(SERVICE_SOURCES
  ${PROJ_ROOT}/src/StateManager.cpp
  ${PROJ_ROOT}/src/DegradationManager.cpp
  ${PROJ_ROOT}/src/SystemMonitor.cpp
  ${PROJ_ROOT}/src/OtaUpdater.cpp
  ${PROJ_ROOT}/src/TimeClientHelper.cpp
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
)

# Link gcov
target_link_libraries(test_runner gcov)

# Test command
add_test(NAME pool_controller_test COMMAND test_runner)
