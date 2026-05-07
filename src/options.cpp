// =============================================================================
// options.cpp — Options::validate() implementation.
// =============================================================================
//
// validate() walks every field of an Options instance and returns the
// first constraint violation as an Error. A populated return value means
// "this configuration is invalid for these reasons"; std::nullopt means
// "every field is in range".
//
// The function is intentionally simple and short:
//   * checks are ordered from cheap to expensive
//   * each check returns immediately on failure (we don't accumulate
//     multiple errors — the caller fixes the first issue and re-validates)
//   * the source location of the failing check is captured via HH_HERE
//     so logs/tests can pinpoint exactly which constraint failed
// =============================================================================

#include "heisenheap/options.h"

namespace heisenheap {

// Anonymous namespace = "private to this translation unit". Equivalent
// to C's `static`. Used for helpers that should not be visible to other
// .cpp files even at link time.
namespace {

/// True iff `v` is a non-zero power of two (1, 2, 4, 8, 16, ...).
///
/// The bit trick `v & (v - 1)` is zero exactly for powers of two:
///   binary 1000 & 0111 = 0000   (8 is a power of two)
///   binary 1010 & 1001 = 1000   (10 is not)
constexpr bool is_power_of_two(std::uint32_t v) noexcept {
    return v != 0 && (v & (v - 1u)) == 0u;
}

/// Hard upper bound on slot_pages_. 64 pages * 4 KiB = 256 KiB per slot,
/// which is more than any sub-page-class allocator should need. The cap
/// stops accidental misconfiguration from reserving gigabytes of VM.
constexpr std::uint32_t kSlotPagesMax = 64u;

}  // namespace

std::optional<Error> Options::validate() const noexcept {
    // sample_rate_ < 1 would mean "sample fewer than one in N", which
    // makes no statistical sense. sample_rate_ == 1 means "sample every
    // allocation" (heaviest mode, useful only in tests).
    if (sample_rate_ < 1u) {
        return make_error(ErrorCode::InvalidOptions, HH_HERE);
    }

    // A pool with zero slots cannot guard anything.
    if (slot_count_ < 1u) {
        return make_error(ErrorCode::InvalidOptions, HH_HERE);
    }

    // slot_pages_ must be a power of two (so slot strides are aligned)
    // and capped at kSlotPagesMax. Both conditions in one branch keeps
    // the error code uniform — callers don't need to differentiate.
    if (!is_power_of_two(slot_pages_) || slot_pages_ > kSlotPagesMax) {
        return make_error(ErrorCode::InvalidOptions, HH_HERE);
    }

    // A zero-size routing threshold would route every allocation to
    // "not sampled", making the allocator a no-op.
    if (max_alloc_size_ == 0u) {
        return make_error(ErrorCode::InvalidOptions, HH_HERE);
    }

    // The quarantine cannot hold more entries than the pool has slots.
    if (quarantine_capacity_ > slot_count_) {
        return make_error(ErrorCode::InvalidOptions, HH_HERE);
    }

    // All fields validated. Returning std::nullopt is the "OK" signal
    // for std::optional<Error>.
    return std::nullopt;
}

}  // namespace heisenheap
