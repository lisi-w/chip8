cmake_minimum_required (VERSION 3.20)
project (yachip8 C CXX)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

cmake_host_system_information(RESULT HNAME QUERY HOSTNAME)

if ("${HNAME}" STREQUAL "de1-soc")
  set(ISDE1SOC ON CACHE BOOL "Are we running on FPGA or desktop?")
  message(STATUS "Detected running on FPGA; not compiling tests.")
else()
  set(ISDE1SOC OFF CACHE BOOL "Are we running on FPGA or desktop?")
  message(STATUS "Detected running on desktop.")
endif()

set(COMPILE_SWPROTO OFF CACHE BOOL "Compile software prototype and tests.")

# GoogleTest snippet stolen from
# https://cmake.org/cmake/help/latest/module/FetchContent.html
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        "release-1.11.0"
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable is to be set in subdirs as needed.

set(WARNING_LIST
  -Wall
  -Wextra
  -pedantic
  -Werror
  -Wno-unused
  -Wno-missing-field-initializers
)
string(REPLACE ";" " " WARNING_FLAGS "${WARNING_LIST}")

if (COMPILE_SWPROTO)
  add_subdirectory (swproto)
endif()

# add_subdirectory (sw)
add_subdirectory (hw)

