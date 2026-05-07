// Inspired by: portable compiler-intrinsic wrappers used in LLVM, Abseil,
// Folly. Simplified by: only the macros heisenheap actually needs.
//
// =============================================================================
// What this header is for
// =============================================================================
//
// Different C++ compilers (Clang, GCC, MSVC) expose the same low-level
// optimisation hints under different spellings:
//
//   * Clang/GCC: __builtin_expect(x, 1)        // tell the CPU "x is usually true"
//   * MSVC:      no equivalent before C++20
//
//   * Clang/GCC: __attribute__((always_inline))   // force inlining
//   * MSVC:      __forceinline
//
// Sprinkling those raw spellings throughout the codebase would make every
// hot-path file noisy and would force every translation unit that needs an
// optimisation hint to know about every supported compiler.
//
// Instead, we introduce one macro per concept (HH_LIKELY, HH_FORCE_INLINE,
// ...). The body of each macro picks the right compiler-specific spelling
// based on which compiler is currently building the code. Call sites just
// write "HH_LIKELY(cond)" and the right thing happens everywhere.
//
// Adding a new compiler only requires extending the branches in this
// file — every call site keeps working unchanged.
// =============================================================================
#pragma once

#if defined(__clang__) || defined(__GNUC__)
// ----- Clang and GCC path -----
//
// __builtin_expect(x, value) returns x but tells the compiler "I expect
// x to equal `value` most of the time". The compiler uses that hint to
// arrange machine code so the common path stays in the CPU's straight
// line of execution (better instruction-cache behaviour, fewer branch
// mispredictions).
//
// The "!!" trick converts whatever expression the user passed into a
// proper 0-or-1 integer so the comparison against 1 / 0 inside
// __builtin_expect is well-defined even when `x` is, say, a pointer.
#define HH_LIKELY(x)   __builtin_expect(!!(x), 1)
#define HH_UNLIKELY(x) __builtin_expect(!!(x), 0)

// Force the function body to be inlined at every call site. The plain
// C++ keyword `inline` is only a *hint*; modern compilers ignore it
// when they think inlining would hurt. always_inline overrides that
// judgement — useful for tiny hot-path helpers (single-instruction
// accessors, branch-prediction shims).
#define HH_FORCE_INLINE inline __attribute__((always_inline))

// The opposite: never inline this function. Useful when we want a
// function symbol to remain present so the debugger can break on it
// (signal handlers, error reporters).
#define HH_NEVER_INLINE __attribute__((noinline))

// C's "restrict" qualifier in C++ form. A "restrict" pointer promises
// the compiler that nothing else in the function will read or write
// through the same address. The compiler can use that promise to
// reorder loads/stores more aggressively. Use sparingly and carefully.
#define HH_RESTRICT __restrict__

// Tells the compiler that control flow can never reach this point.
// Used after exhaustive switch statements over an `enum class` so the
// compiler knows it doesn't need to emit a default branch.
#define HH_UNREACHABLE() __builtin_unreachable()

// Lets us emit a `#pragma` from inside a macro — useful for diagnostic
// suppression around tricky inline code.
#define HH_PRAGMA(x) _Pragma(#x)

#elif defined(_MSC_VER)
// ----- MSVC path -----
//
// Wired up so call sites stay portable; not exercised by the current
// macOS/Linux test matrix.
#define HH_LIKELY(x)     (x)  // MSVC has no direct equivalent
#define HH_UNLIKELY(x)   (x)
#define HH_FORCE_INLINE  __forceinline
#define HH_NEVER_INLINE  __declspec(noinline)
#define HH_RESTRICT      __restrict
#define HH_UNREACHABLE() __assume(0)
#define HH_PRAGMA(x)     __pragma(x)

#else
// Any other toolchain (older xlC, Intel classic, exotic embedded
// compilers) is unsupported. We fail at build time rather than silently
// producing a slower binary because the optimisation hints disappeared.
#error "Unsupported compiler: extend heisenheap/compiler.h."
#endif
