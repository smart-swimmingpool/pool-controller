cmake_minimum_required(VERSION 3.16)
project(pool_controller_relay_safety_tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fno-omit-frame-pointer")

add_definitions(-DNORVI_AE01_R)

set(PROJ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# IMPORTANT: src/ is listed FIRST. RelayModuleNode.cpp, RuleAuto.cpp,
# RuleBoost.cpp, RuleTimer.cpp and Timer.cpp are compiled directly from
# src/ (not copied to a wrapper dir), so their own quote-includes
# (`#include "RuleAuto.hpp"` etc.) resolve to their sibling production
# headers via the compiler's same-directory-first rule regardless of -I
# order. Listing src/ first here only matters for THIS project's own
# test_relay_safety.cpp, which has no sibling headers of its own and must
# fall through to -I — we want it to see the REAL production headers, not
# the simplified/offset-hacked mocks used by the main test_runner target
# (mixing the two would be an ODR violation).
include_directories(
  ${PROJ_ROOT}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
  ${PROJ_ROOT}/lib
)

set(PRODUCTION_SOURCES
  ${PROJ_ROOT}/src/RelayModuleNode.cpp
  ${PROJ_ROOT}/src/RuleAuto.cpp
  ${PROJ_ROOT}/src/RuleBoost.cpp
  ${PROJ_ROOT}/src/RuleTimer.cpp
  ${PROJ_ROOT}/src/Timer.cpp
  ${PROJ_ROOT}/src/LogCapture.cpp
)

set(STUB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs.cpp
)

set(TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_relay_safety.cpp
)

add_executable(test_relay_safety
  ${PRODUCTION_SOURCES}
  ${STUB_SOURCES}
  ${TEST_SOURCES}
)

add_test(NAME relay_safety_test COMMAND test_relay_safety)
