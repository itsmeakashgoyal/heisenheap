// =============================================================================
// platform.h — compile-time platform and architecture detection.
// =============================================================================
//
// The compiler defines a handful of standard predefined macros that
// identify the target operating system (__APPLE__, __linux__, _WIN32)
// and CPU architecture (__aarch64__, __x86_64__). Different parts of
// heisenheap need to make platform-specific choices — for example:
//
//   * the VM layer chooses between mmap (POSIX) and VirtualAlloc (Windows)
//   * the stack walker chooses between frame-pointer walking (ARM64)
//     and unwind tables (x86_64) when the implementations differ
//
// Rather than scatter `#ifdef __APPLE__` checks across the codebase, we
// fold them into one place and define our own well-named macros
// (HH_PLATFORM_MACOS, HH_ARCH_ARM64). Every other file then writes:
//
//     #if HH_PLATFORM_MACOS
//         ...
//     #endif
//
// If a platform we don't yet support is detected, we fail loudly at
// build time rather than producing a silently broken binary.
// =============================================================================
#pragma once

// ---- Operating system ------------------------------------------------------
#if defined(__APPLE__)
#define HH_PLATFORM_APPLE 1  // any Apple platform
#define HH_PLATFORM_MACOS 1  // specifically macOS (we don't target iOS)
#define HH_PLATFORM_POSIX 1  // POSIX APIs (sigaction, mmap, ...) are available
#elif defined(__linux__)
#define HH_PLATFORM_LINUX 1
#define HH_PLATFORM_POSIX 1
#else
// Building on anything that isn't macOS or Linux fails here rather
// than producing surprises later.
#error "Unsupported platform: only macOS and Linux are supported."
#endif

// ---- CPU architecture ------------------------------------------------------
//
// Architecture matters mainly for the stack walker (frame-pointer layout
// differs between AArch64 and x86_64) and for any inline assembly we
// might need. We only bake in the two architectures we actually run on.
#if defined(__aarch64__)
#define HH_ARCH_ARM64 1
#elif defined(__x86_64__)
#define HH_ARCH_X86_64 1
#else
#error "Unsupported architecture."
#endif
