// Inspired by: GWP-ASan's options-error reporting. Simplified by: a
// single 16-bit enum + a static description table; no exceptions, no
// dynamic strings.
//
// =============================================================================
// What this header is for
// =============================================================================
//
// Heisenheap is allocator infrastructure: when something goes wrong, we
// can't allocate a new std::string to describe it (the system is in the
// middle of allocating!). Throwing a C++ exception is also off the table
// because exceptions can themselves allocate, and library code that runs
// inside an interposed `malloc` call must be `noexcept` end-to-end.
//
// So we treat errors as plain *data*:
//
//     std::optional<Allocation>      try_allocate(...);   // empty == failure
//     std::optional<Error>           validate() const;    // empty == OK
//
// The `Error` value is a tiny 3-field struct:
//   * an `ErrorCode` saying *what* went wrong
//   * a pointer to a static C-string explaining the code in human terms
//   * a `SourceLocation` saying *where* in our code the error was raised
//
// Every code maps to a fixed string in `src/error.cpp::describe()`. The
// caller never has to allocate; the message lives in read-only data.
// =============================================================================
#pragma once

#include <cstdint>

#include "heisenheap/source_location.h"

namespace heisenheap {

/// The closed set of failure reasons heisenheap's public API can return.
///
/// We use `enum class` (a "scoped enum") rather than a plain C-style
/// enum for two reasons:
///   1. Strong typing — `ErrorCode::PoolExhausted` cannot be silently
///      converted to or compared against an int.
///   2. Scoping — every enumerator must be qualified
///      (`ErrorCode::None` rather than just `None`), preventing name
///      clashes with macros from system headers.
///
/// The underlying integer is `std::uint16_t` so the value is cheap to
/// pass and stored compactly inside `Error` (which lives in user code).
enum class ErrorCode : std::uint16_t {
    None = 0,                   ///< sentinel: not an error
    Uninitialized,              ///< Heisenheap::create() was never called
    AlreadyInitialized,         ///< second call to create() while still running
    InvalidOptions,             ///< Options::validate() found a bad field
    SizeZero,                   ///< caller asked for 0 bytes
    SizeAboveMaximum,           ///< requested size > Options::max_alloc_size_
    AlignmentNotPowerOfTwo,     ///< alignment must be 1, 2, 4, 8, 16, ...
    AlignmentTooLarge,          ///< alignment > implementation maximum
    NotSampled,                 ///< sampler said "skip this allocation"
    NotOwned,                   ///< pointer doesn't belong to heisenheap
    DoubleFree,                 ///< pointer is in the quarantine list
    InvalidFree,                ///< pointer is not at a slot's user_ptr
    PoolExhausted,              ///< no free slots in the guarded pool
    LargeCapacityExhausted,     ///< no free records in the large allocator
    OutOfMemory,                ///< pal::vm reservation failed
    DisabledOnCurrentThread,    ///< ScopedDisable is active on this thread
    Internal,                   ///< logic bug — should never happen in shipped code
};

/// Value type returned (wrapped in std::optional) by APIs that can fail
/// with context richer than "yes/no".
///
/// Three fields, sized to fit in three pointers on 64-bit platforms:
///   * `code_`    — what went wrong (closed enum, see above)
///   * `message_` — human-readable string for the code (static, never freed)
///   * `where_`   — the file/line/function in *our* code that raised the error
///
/// Callers usually inspect `code_` programmatically and surface
/// `message_` to a log. `where_` is mainly for our own debugging.
struct Error {
    /// Which failure reason this error represents.
    ErrorCode code_ = ErrorCode::None;

    /// Pointer to a static (non-allocating) human-readable string.
    /// Set by `make_error()` from the result of `describe(code_)`.
    const char* message_ = nullptr;

    /// Where in heisenheap's source the error was raised. Set by
    /// `make_error(code, HH_HERE)`. Default-constructed if the caller
    /// did not pass a location.
    SourceLocation where_ = {};

    /// Quick predicate: is this the "no error" sentinel?
    [[nodiscard]] constexpr bool ok() const noexcept {
        return code_ == ErrorCode::None;
    }

    /// Lets callers write `if (error) { ... }`. Returns true when the
    /// Error represents a real failure (code is not None).
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return !ok();
    }
};

/// Returns a non-null pointer to a fixed C-string describing `code`.
///
/// The returned string lives in the binary's read-only data segment,
/// so callers may store the pointer indefinitely and must never free it.
/// Implementation lives in `src/error.cpp` to keep the description table
/// out of every translation unit that includes this header.
[[nodiscard]] const char* describe(ErrorCode code) noexcept;

/// Convenience constructor: builds an Error with the right message
/// looked up from the description table, plus an optional source
/// location captured via HH_HERE at the call site.
///
///     return make_error(ErrorCode::PoolExhausted, HH_HERE);
[[nodiscard]] inline Error make_error(
        ErrorCode code,
        SourceLocation where = {}) noexcept {
    return Error{code, describe(code), where};
}

}  // namespace heisenheap
