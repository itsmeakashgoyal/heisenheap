// Inspired by: std::source_location (C++20). Simplified by: a POD struct
// captured via a preprocessor macro, since C++17 has no equivalent of
// std::source_location::current().
//
// =============================================================================
// What this header is for
// =============================================================================
//
// When something goes wrong in a library — an out-of-memory error, an
// invalid free, a misconfigured option — the library's caller wants to
// know not just "what" but "where". The classic answer is for every error
// site to manually pass `__FILE__` and `__LINE__`:
//
//     return Error{ErrorCode::PoolExhausted, __FILE__, __LINE__};
//
// That's noisy. C++20 introduced `std::source_location` which captures the
// caller's location automatically when used as a default argument. Until
// we can rely on C++20, we emulate the same idea with a tiny POD struct
// plus one preprocessor macro (HH_HERE).
//
// Usage:
//
//     auto err = make_error(ErrorCode::PoolExhausted, HH_HERE);
//     // err.where_.file_   == "src/pool.cpp"
//     // err.where_.line_   == 42
//     // err.where_.function_ == "SlotPool::try_allocate"
//
// The macro form is required: only the C preprocessor can expand __FILE__
// and __LINE__ to the location of the *caller*. A regular function would
// always report its own definition site.
// =============================================================================
#pragma once

#include <cstdint>

namespace heisenheap {

/// A snapshot of a source code location: which file, which function,
/// which line, which column. Constructed by the HH_HERE macro at the
/// call site.
///
/// Trailing underscores on every field follow the project style rule
/// "all data members end with _" (see STYLE.md). The fields are public
/// so the type can be used as a C++17 aggregate (initialised by
/// positional `{...}` syntax).
struct SourceLocation {
    /// Path of the source file as the preprocessor saw it. Set by
    /// __FILE__ when constructed via HH_HERE; empty string otherwise.
    const char* file_ = "";

    /// Name of the enclosing function. Set by __func__ at the macro
    /// expansion site. Spelling differs by compiler (e.g. "TestBody"
    /// inside a gtest), but always non-null.
    const char* function_ = "";

    /// 1-based source line number, set by __LINE__.
    std::uint_least32_t line_ = 0;

    /// 1-based column number. Always 0 in C++17 — the standard
    /// preprocessor exposes no __COLUMN__. We surface the field so the
    /// type matches std::source_location and the future migration is
    /// mechanical.
    std::uint_least32_t column_ = 0;
};

}  // namespace heisenheap

// HH_HERE expands to a SourceLocation value initialised with the caller's
// file / function / line. Has to be a macro because only the preprocessor
// can capture __FILE__ / __LINE__ at the *call site*; an inline function
// would always report its own definition site.
//
// We use C++17 positional aggregate initialisation (`SourceLocation{a,b,c,d}`)
// rather than designated initialisers (`{.file_=a, ...}`) because designated
// init is C++20-only. The positional form depends on field declaration
// order in the struct above.
#define HH_HERE                                                       \
    ::heisenheap::SourceLocation {                                    \
        __FILE__,                                                     \
        static_cast<const char*>(__func__),                           \
        static_cast<std::uint_least32_t>(__LINE__),                   \
        0u                                                            \
    }
