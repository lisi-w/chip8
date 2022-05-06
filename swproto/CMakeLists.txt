FetchContent_MakeAvailable(googletest)

find_package(SFML COMPONENTS graphics REQUIRED)

set(SWPROTO_LIBRARY_SOURCES statemachine.cpp statemachine.hpp font.cpp font.hpp)
add_executable(emulator emulator.cpp ${SWPROTO_LIBRARY_SOURCES})
set_property(TARGET emulator PROPERTY CXX_STANDARD 20)
set_property(TARGET emulator PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(emulator sfml-graphics)


add_executable(statemachine_test statemachine_test.cpp ${SWPROTO_LIBRARY_SOURCES})
target_link_libraries(statemachine_test gtest_main)
set_property(TARGET statemachine_test PROPERTY CXX_STANDARD 20)
set_property(TARGET statemachine_test PROPERTY CXX_STANDARD_REQUIRED ON)

