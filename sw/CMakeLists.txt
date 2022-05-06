
add_executable(userspace keyboard_test.cpp keyboard.cpp keyboard.hpp)
#target_compile_options(hw_test PRIVATE ${WARNING_LIST})
set_property(TARGET userspace PROPERTY CXX_STANDARD 20)
set_property(TARGET userspace PROPERTY CXX_STANDARD_REQUIRED ON)
target_include_directories(userspace PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(kerneldir "" CACHE STRING "Path to the kernel build directory")
if("${kerneldir}" STREQUAL "")
  execute_process(COMMAND uname -r OUTPUT_VARIABLE uname_r
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(kerneldir "/lib/modules/${uname_r}/build")
endif()
find_file(kernel_makefile NAMES Makefile
                          PATHS ${kerneldir} NO_DEFAULT_PATH)
if(NOT kernel_makefile)
  message(FATAL_ERROR "There is no Makefile in kerneldir!")
endif()

# Generate the Kbuild file through cmake.
FILE(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/Kbuild "obj-m := hello.o")


set(module_cmd ${CMAKE_MAKE_PROGRAM} -C ${kerneldir} M=${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(OUTPUT mymodule.ko
  COMMAND ${module_cmd} modules
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${module_sources} ${CMAKE_CURRENT_BINARY_DIR}/Kbuild
  VERBATIM)
add_custom_target(module DEPENDS mymodule.ko)
add_custom_target(module-clean COMMAND ${module_cmd} clean)


