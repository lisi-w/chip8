cmake_minimum_required (VERSION 3.16)
project (yachip8 C CXX)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused)
endif()


add_subdirectory (keyboard)

