// Inspired by: GWP-ASan's AllocationMetadata struct. Simplified by:
// a Span<std::byte> accessor in lieu of raw (ptr, size) plumbing at
// call sites.
//
// =============================================================================
// What this header is for
// =============================================================================
//
// The public allocation API of heisenheap looks like:
//
//     std::optional<Allocation>     try_allocate(size_t size, size_t align);
//     std::optional<Reallocation>   try_reallocate(void* p, size_t new_size, ...);
//     std::optional<Deallocation>   try_deallocate(void* p);
//
// Each of those returns a small value type carrying just enough metadata
// for the caller to:
//   * find the bytes (`ptr_`)
//   * know how much they may safely write (`usable_size_`)
//   * tell whether a guard was actually applied (`was_guarded_`)
//   * detect whether a realloc moved the buffer (`moved_`)
//
// These types are deliberately plain "aggregates" — no constructors, no
// inheritance, no virtual functions. That means they:
//   * cost nothing at runtime
//   * can be initialised with positional `{...}` syntax
//   * are trivially copyable, so passing them by value is a register-wide move
//
// They are populated by the allocator and consumed by the caller. The
// allocator owns the memory referenced by `ptr_`; the caller borrows it
// until the matching `try_deallocate`.
// =============================================================================
#pragma once

#include <cstddef>      // std::size_t, std::byte

#include "heisenheap/span.h"

namespace heisenheap {

/// Result of a successful allocation.
///
/// Returned by `Heisenheap::try_allocate` and `try_callocate`. The
/// caller owns the right to read/write `ptr_[0 .. usable_size_)` until
/// `try_deallocate(ptr_)` is called.
struct Allocation {
    /// Pointer to the first usable byte. Aligned to at least the
    /// alignment requested in the allocate call. nullptr only when the
    /// Allocation is the default-constructed "invalid" sentinel.
    void* ptr_ = nullptr;

    /// The size the caller asked for, in bytes.
    std::size_t requested_size_ = 0;

    /// The size the caller may *actually* write, in bytes. Always
    /// `>= requested_size_`. May be larger than requested when the
    /// allocator pads to page geometry (the extra bytes are still
    /// guarded so writing past `usable_size_` will hit a guard page).
    std::size_t usable_size_ = 0;

    /// Type-safe view of the allocated bytes. Saves callers from
    /// `static_cast<std::byte*>(ptr_)` plumbing every time they want
    /// to memcpy / memset / iterate over the buffer.
    [[nodiscard]] Span<std::byte> bytes() const noexcept {
        return Span<std::byte>(static_cast<std::byte*>(ptr_), usable_size_);
    }

    /// Quick predicate: was the allocation actually performed?
    /// (i.e. `ptr_ != nullptr`). False for default-constructed values.
    [[nodiscard]] bool valid() const noexcept { return ptr_ != nullptr; }
};

/// Result of a successful deallocation.
///
/// Returned by `Heisenheap::try_deallocate`. The caller usually doesn't
/// need it, but the fields are useful for metrics and tests:
///   * `was_guarded_` tells you whether the freed pointer was managed
///     by heisenheap (true) or whether the call was a graceful no-op
///     because the pointer wasn't ours (false).
///   * `freed_size_` is the `usable_size_` of the slot that was freed.
struct Deallocation {
    bool        was_guarded_ = false;
    std::size_t freed_size_  = 0;
};

/// Result of a successful reallocation.
///
/// Returned by `Heisenheap::try_reallocate`. Mirrors `Allocation` but
/// also records:
///   * the previous pointer the caller passed in (`old_ptr_`)
///   * whether the data was copied to a new buffer (`moved_ == true`)
///     or grown in place (`moved_ == false`)
struct Reallocation {
    /// The (possibly new) pointer to the resized buffer.
    void* ptr_ = nullptr;

    /// The original pointer the caller passed to try_reallocate.
    void* old_ptr_ = nullptr;

    /// Usable bytes at `ptr_` after the realloc.
    std::size_t usable_size_ = 0;

    /// True if `ptr_ != old_ptr_` and the bytes were copied to a new
    /// guarded location. False if the realloc grew/shrunk the existing
    /// buffer in place.
    bool moved_ = false;

    /// Type-safe view of the (possibly new) buffer. Same idea as
    /// `Allocation::bytes()`.
    [[nodiscard]] Span<std::byte> bytes() const noexcept {
        return Span<std::byte>(static_cast<std::byte*>(ptr_), usable_size_);
    }
};

}  // namespace heisenheap
