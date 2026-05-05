include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# Prevent GoogleTest from overriding our compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# hh_tests: all unit/integration tests live in one binary for now.
# We will add more source files here later.
add_executable(hh_tests
    placeholder.cpp
)

target_link_libraries(hh_tests PRIVATE
    heisenheap
    GTest::gtest_main
)

apply_compiler_flags(hh_tests)

include(GoogleTest)
gtest_discover_tests(hh_tests)
