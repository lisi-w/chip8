add_executable(swimp main.cpp statemachine.cpp statemachine.hpp font.cpp font.hpp)
set_property(TARGET swimp PROPERTY CXX_STANDARD 20)
set_property(TARGET swimp PROPERTY CXX_STANDARD_REQUIRED ON)
target_include_directories(swimp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
