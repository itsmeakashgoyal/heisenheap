// =============================================================================
// export.h — symbol-visibility macro for shared-library builds.
// =============================================================================
//
// On most Unix-like systems, every C++ symbol is "exported" from a
// shared library by default. That's noisy: internal helper functions
// end up in the library's public ABI even though no caller is supposed
// to use them. The cleaner pattern is "hidden by default, mark the
// few public symbols explicitly":
//
//     extern "C" HH_API int public_function();
//
// We tag every symbol the public API exposes with `HH_API`. When
// heisenheap is built as a static library (the default), the macro
// expands to nothing and there's no visibility annotation — every
// symbol is reachable to anyone who links against the .a file.
//
// When somebody builds heisenheap as a shared library
// (`cmake -DHH_BUILD_SHARED=ON`, not enabled by default), we want
// exactly the HH_API-marked symbols to appear in the shared library's
// symbol table and everything else to be hidden. The compiler flag
// `-fvisibility=hidden` does the "hide everything by default" half;
// the `__attribute__((visibility("default")))` on HH_API does the
// "expose this one" half.
// =============================================================================
#pragma once

#if defined(HH_BUILDING_SHARED) || defined(HH_SHARED)
    // Shared-library build path: explicitly mark this symbol as exported.
    #define HH_API __attribute__((visibility("default")))
#else
    // Static-library build path: nothing to do, every symbol is already
    // visible to whoever links against libheisenheap.a.
    #define HH_API
#endif
