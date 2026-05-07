include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# Prevent GoogleTest from overriding our compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# hh_tests: all unit/integration tests live in one binary.
add_executable(hh_tests
    test_span.cpp
    test_source_location.cpp
    test_error.cpp
    test_allocation.cpp
    test_options.cpp
)

target_link_libraries(hh_tests PRIVATE
    heisenheap
    GTest::gtest_main
)

apply_compiler_flags(hh_tests)

include(GoogleTest)
gtest_discover_tests(hh_tests)
