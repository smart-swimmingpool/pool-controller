cmake_minimum_required(VERSION 3.16)
project(pool_controller_config_persistence_tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fno-omit-frame-pointer")

set(PROJ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# The real ConfigManager is compiled from src/ against the Preferences mock,
# whose fault injection lets a single put fail. src/ comes before mocks/ so the
# test sees the production header, not the ConfigManager mock used by
# test_runner.
include_directories(
  ${PROJ_ROOT}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
)

add_executable(test_config_persistence
  ${PROJ_ROOT}/src/ConfigManager.cpp
  ${PROJ_ROOT}/src/LogCapture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_config_persistence.cpp
)

add_test(NAME config_persistence_test COMMAND test_config_persistence)
