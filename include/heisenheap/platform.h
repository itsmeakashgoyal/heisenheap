#pragma once

// Detect the target platform at compile time.
// Every platform-specific #ifdef in the codebase should check these macros,
// not raw __APPLE__ / __linux__, so we have one central place to extend.

#if defined(__APPLE__)
    #define HH_PLATFORM_APPLE  1
    #define HH_PLATFORM_MACOS  1
    #define HH_PLATFORM_POSIX  1
#elif defined(__linux__)
    #define HH_PLATFORM_LINUX  1
    #define HH_PLATFORM_POSIX  1
#else
    #error "Unsupported platform: only macOS and Linux are supported."
#endif

// Architecture
#if defined(__aarch64__)
    #define HH_ARCH_ARM64 1
#elif defined(__x86_64__)
    #define HH_ARCH_X86_64 1
#else
    #error "Unsupported architecture."
#endif
