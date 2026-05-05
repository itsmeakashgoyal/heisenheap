function(apply_compiler_flags target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Werror
        # Required for frame-pointer stack walking
        # Without this the compiler reuses the frame pointer register for
        # general-purpose computation and we cannot reconstruct call stacks.
        -fno-omit-frame-pointer
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Debug>:-O0 -g>
    )
endfunction()
