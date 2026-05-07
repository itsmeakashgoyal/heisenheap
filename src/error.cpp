// =============================================================================
// error.cpp — implementation of the static description table for ErrorCode.
// =============================================================================
//
// Every enumerator in `ErrorCode` (see error.h) maps to a fixed,
// non-allocating, read-only C-string. We keep the table in a translation
// unit (this .cpp file) rather than inline in the header so:
//
//   1. Every TU that includes error.h doesn't drag in the strings.
//   2. There is exactly one canonical place to update wording.
//   3. The compiler can confirm the switch is exhaustive (-Werror would
//      flag an unhandled enumerator).
// =============================================================================

#include "heisenheap/error.h"

#include "heisenheap/compiler.h"  // for HH_UNREACHABLE()

namespace heisenheap {

const char* describe(ErrorCode code) noexcept {
    // The switch must cover every value of ErrorCode. If a new
    // enumerator is added in the header without being added here,
    // -Wswitch will warn (and -Werror will turn that into a build
    // failure), keeping the table in sync with the enum.
    switch (code) {
        case ErrorCode::None:
            return "no error";
        case ErrorCode::Uninitialized:
            return "heisenheap is not initialized";
        case ErrorCode::AlreadyInitialized:
            return "heisenheap is already initialized";
        case ErrorCode::InvalidOptions:
            return "invalid options";
        case ErrorCode::SizeZero:
            return "allocation size must be greater than zero";
        case ErrorCode::SizeAboveMaximum:
            return "allocation size exceeds configured maximum";
        case ErrorCode::AlignmentNotPowerOfTwo:
            return "alignment must be a power of two";
        case ErrorCode::AlignmentTooLarge:
            return "alignment exceeds supported maximum";
        case ErrorCode::NotSampled:
            return "allocation was not sampled";
        case ErrorCode::NotOwned:
            return "pointer is not owned by heisenheap";
        case ErrorCode::DoubleFree:
            return "pointer was already freed";
        case ErrorCode::InvalidFree:
            return "free of an invalid pointer";
        case ErrorCode::PoolExhausted:
            return "guarded slot pool is exhausted";
        case ErrorCode::LargeCapacityExhausted:
            return "large allocator capacity is exhausted";
        case ErrorCode::OutOfMemory:
            return "out of virtual memory";
        case ErrorCode::DisabledOnCurrentThread:
            return "heisenheap is disabled on the current thread";
        case ErrorCode::Internal:
            return "internal error";
    }
    // Unreachable: every enumerator is handled above. This both silences
    // GCC's "control reaches end of non-void function" warning and lets
    // the compiler skip emitting the implicit fall-through code.
    HH_UNREACHABLE();
}

}  // namespace heisenheap
