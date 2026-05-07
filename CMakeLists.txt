cmake_minimum_required(VERSION 3.24)
project(heisenheap VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/CompilerFlags.cmake)
include(cmake/Sanitizers.cmake)

option(HH_BUILD_TESTS  "Build test suite"       ON)
option(HH_BUILD_SHARED "Build as shared library" OFF)

add_library(heisenheap STATIC
    src/allocator.cpp
    src/error.cpp
    src/options.cpp
)

target_include_directories(heisenheap
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

apply_compiler_flags(heisenheap)

if(APPLE)
    target_link_libraries(heisenheap PRIVATE "-framework Foundation")
endif()

if(HH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(examples)
