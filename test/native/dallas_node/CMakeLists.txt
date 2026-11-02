cmake_minimum_required(VERSION 3.16)
project(pool_controller_dallas_node_tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fno-omit-frame-pointer")

set(PROJ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# The real DallasTemperatureNode is compiled from src/ against a programmable
# bus fake. fakes/ comes first so <DallasTemperature.h> resolves to the fake
# instead of the inert mock; src/ comes before mocks/ so the test sees the
# production headers, not the simplified node mock used by test_runner.
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/fakes
  ${PROJ_ROOT}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
)

add_executable(test_dallas_node
  ${PROJ_ROOT}/src/DallasTemperatureNode.cpp
  ${PROJ_ROOT}/src/SensorSlots.cpp
  ${PROJ_ROOT}/src/LogCapture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_dallas_node.cpp
)

add_test(NAME dallas_node_test COMMAND test_dallas_node)
