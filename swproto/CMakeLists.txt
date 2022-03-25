set(SWIMP_LIBRARY_SOURCES statemachine.cpp statemachine.hpp font.cpp font.hpp)
add_executable(swimp main.cpp ${SWIMP_LIBRARY_SOURCES})
set_property(TARGET swimp PROPERTY CXX_STANDARD 20)
set_property(TARGET swimp PROPERTY CXX_STANDARD_REQUIRED ON)

add_executable(statemachine_test statemachine_test.cpp ${SWIMP_LIBRARY_SOURCES})
target_link_libraries(statemachine_test gtest_main)
set_property(TARGET statemachine_test PROPERTY CXX_STANDARD 20)
set_property(TARGET statemachine_test PROPERTY CXX_STANDARD_REQUIRED ON)

