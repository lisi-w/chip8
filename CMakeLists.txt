cmake_minimum_required (VERSION 3.20)
project (yachip8 C CXX)

# GoogleTest snippet stolen from https://cmake.org/cmake/help/latest/module/FetchContent.html
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        "release-1.11.0"
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused)
endif()


add_subdirectory (swimp)

