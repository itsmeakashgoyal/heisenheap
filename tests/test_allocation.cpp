// Tests for include/heisenheap/allocation.h.
//
// Allocation / Deallocation / Reallocation are pure value types (POD
// aggregates) so the test surface is small: we confirm aggregate
// initialisation works, the helper accessors return sensible values,
// and the layout matches what the rest of the codebase assumes.
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "heisenheap/allocation.h"

using heisenheap::Allocation;
using heisenheap::Deallocation;
using heisenheap::Reallocation;

// A default-constructed Allocation is the "invalid" sentinel: ptr_ is
// null, sizes are zero, valid() returns false, and bytes() yields an
// empty span. APIs that fail return this state by default.
TEST(Allocation, DefaultConstructedIsInvalidAndEmpty) {
    Allocation a{};
    EXPECT_FALSE(a.valid());
    EXPECT_TRUE(a.bytes().empty());
    EXPECT_EQ(a.requested_size_, 0u);
    EXPECT_EQ(a.usable_size_, 0u);
}

// bytes() must produce a Span<std::byte> of length usable_size_ that
// points at the same memory as ptr_. This is the helper most callers
// use for byte-level access (memcpy/memset/iteration).
TEST(Allocation, BytesViewMatchesPtrAndUsableSize) {
    std::uint8_t storage[16] = {};
    Allocation a{storage, /*requested_size=*/10, /*usable_size=*/16};
    EXPECT_TRUE(a.valid());
    auto bytes = a.bytes();
    EXPECT_EQ(bytes.size(), 16u);
    EXPECT_EQ(static_cast<void*>(bytes.data()), static_cast<void*>(storage));
}

// `moved_` exists so callers can detect realloc that copied to a new
// buffer (in which case any pointers into the old buffer are invalid).
// We explicitly set moved_ to test both paths.
TEST(Reallocation, MovedReflectsPtrChange) {
    int x = 0;
    int y = 0;
    Reallocation moved{&y, &x, 4u, true};
    Reallocation in_place{&x, &x, 4u, false};
    EXPECT_TRUE(moved.moved_);
    EXPECT_FALSE(in_place.moved_);
}

// Plain aggregate-init smoke test. If somebody adds a private member
// or a constructor, this brace-init form will fail to compile.
TEST(Deallocation, AggregateInit) {
    Deallocation d{true, 64};
    EXPECT_TRUE(d.was_guarded_);
    EXPECT_EQ(d.freed_size_, 64u);
}

// std::is_aggregate_v reports true only for "plain" structs without
// virtuals or explicit constructors. We rely on aggregate-ness because:
//   * positional `{...}` init must keep working
//   * trivial copy / move semantics make passing them by value cheap
TEST(Allocation, IsAggregate) {
    static_assert(std::is_aggregate_v<Allocation>);
    static_assert(std::is_aggregate_v<Deallocation>);
    static_assert(std::is_aggregate_v<Reallocation>);
}

// Layout sanity check: on a 64-bit target Allocation is exactly three
// pointer-sized fields (ptr_, requested_size_, usable_size_). This
// catches accidental size growth (e.g. someone sneaks in a vtable).
TEST(Allocation, SizeIsThreePointersOn64Bit) {
    if constexpr (sizeof(void*) == 8) {
        static_assert(sizeof(Allocation) == 3 * sizeof(void*),
                      "Allocation layout drifted on 64-bit target");
    }
    SUCCEED();
}
