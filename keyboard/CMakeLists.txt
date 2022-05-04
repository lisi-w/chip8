set(KEYBOARD_LIB_SRCS keyboard.cpp keyboard.hpp)
add_executable(keyboard_test keyboard_test.cpp $(KEYBOARD_LIB_SRCS))
set_property(TARGET keyboard PROPERTY CXX_STANDARD 20)
set_property(TARGET keyboard PROPERTY CXX_STANDARD_REQUIRED ON)
