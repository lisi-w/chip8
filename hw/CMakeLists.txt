if (ISDE1SOC)

else()
  FetchContent_MakeAvailable(googletest)

  find_package(verilator HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT})
  if (NOT verilator_FOUND)
    message(FATAL_ERROR
      "Verilator not found. Install it or set VERILATOR_ROOT environment variable")
  endif()

  #file(GLOB_RECURSE ${VERILATOR_ROOT} "*.c" "*.h")

  set(hw_test_sources hw_test.hpp main.cpp chip_ram_test.cpp display_ram.cpp)
  list(LENGTH hw_test_sources hw_test_sources_nr)

  set(hw_test_include_dirs "")
  list(LENGTH hw_test_include_dirs hw_test_include_dirs_nr)

  add_executable(hw_test ${hw_test_sources})
  target_link_libraries(hw_test PRIVATE gtest)
  set_target_properties(hw_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)
  # Each verilate() call may have only one top level module which gets turned
  # into a .h file.
  verilate(hw_test TRACE SOURCES chip_ram.sv)
  verilate(hw_test TRACE SOURCES display_ram.sv)
  verilate(hw_test TRACE SOURCES display.sv)
  verilate(hw_test TRACE SOURCES cpu.sv)

  # We have to enable warnings piecemeal so as not to interfere
  # with Verilator.

  # Move verilator includes to system includes.
  get_target_property(
    verilator_hw_test_include_dirs
    hw_test
    INCLUDE_DIRECTORIES
  )
  set_target_properties(hw_test PROPERTIES INCLUDE_DIRECTORIES "")
  target_include_directories(hw_test SYSTEM
    PRIVATE ${verilator_hw_test_include_dirs})

  set(TRACE_DIR "${CMAKE_CURRENT_BINARY_DIR}/traces/"
    CACHE PATH
    "Where to put Verilator test traces.")

  file(MAKE_DIRECTORY "${TRACE_DIR}")
  target_compile_definitions(hw_test PRIVATE TRACE_DIR="${TRACE_DIR}")

  # Enable warnings for our code, only.
  set_property(
    SOURCE ${hw_test_sources}
    APPEND PROPERTY COMPILE_FLAGS " ${WARNING_FLAGS} ")


endif()

