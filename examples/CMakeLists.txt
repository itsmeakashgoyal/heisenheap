# Examples are standalone executables that demonstrate the public API.
# We will populate basic_usage.cpp later once the API exists.
add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE heisenheap)
apply_compiler_flags(basic_usage)
