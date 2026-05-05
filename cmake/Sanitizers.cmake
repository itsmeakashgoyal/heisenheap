option(HH_ASAN "Enable AddressSanitizer" OFF)
option(HH_TSAN "Enable ThreadSanitizer"  OFF)

function(enable_asan target)
    target_compile_options(${target} PRIVATE
        -fsanitize=address
        -fno-omit-frame-pointer
    )
    target_link_options(${target} PRIVATE -fsanitize=address)
endfunction()

function(enable_tsan target)
    target_compile_options(${target} PRIVATE -fsanitize=thread)
    target_link_options(${target} PRIVATE -fsanitize=thread)
endfunction()

# Apply globally if the option is set at configure time:
#   cmake -B build -DHH_ASAN=ON
if(HH_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(HH_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()
