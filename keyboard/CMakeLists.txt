add_executable(keyboard_test keyboard_interface.cpp keyboard.cpp keyboard.hpp)
set_property(TARGET keyboard PROPERTY CXX_STANDARD 20)
set_property(TARGET keyboard PROPERTY CXX_STANDARD_REQUIRED ON)
target_include_directories(keyboard PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
