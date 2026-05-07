// Tests for include/heisenheap/span.h.
//
// We exercise every public Span<T> entry point with simple buffers and
// check both the runtime and constexpr surface. There is no system call
// or threading involved — all behaviour is pure pointer arithmetic.
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <numeric>

#include "heisenheap/span.h"

using heisenheap::Span;

// A default-constructed span must look like (nullptr, 0). This is the
// "no buffer" state used as a return placeholder by some APIs.
TEST(Span, DefaultConstructedIsEmpty) {
    Span<int> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
    EXPECT_EQ(s.data(), nullptr);
    EXPECT_EQ(s.size_bytes(), 0u);
}

// Construct from an explicit (ptr, count) pair and confirm element
// access reaches the same memory the underlying array uses.
TEST(Span, ConstructFromPointerAndSize) {
    int data[5] = {10, 20, 30, 40, 50};
    Span<int> s(data, 5);
    ASSERT_EQ(s.size(), 5u);
    EXPECT_FALSE(s.empty());
    for (std::size_t i = 0; i < s.size(); ++i) {
        EXPECT_EQ(s[i], data[i]);
    }
}

// The C-array overload deduces the size from the array's type, so the
// caller cannot accidentally pass a wrong count.
TEST(Span, ConstructFromCArrayDeducesSize) {
    int data[7]{};
    std::iota(std::begin(data), std::end(data), 1);  // fill with 1..7
    Span<int> s(data);
    EXPECT_EQ(s.size(), 7u);
    EXPECT_EQ(s[0], 1);
    EXPECT_EQ(s[6], 7);
}

// Implicit Span<T> -> Span<const T> conversion is the common
// "make-it-read-only" idiom. The test confirms it compiles (the static
// type system is doing the heavy lifting) and preserves data.
TEST(Span, ImplicitConstToConstSpanConversion) {
    int data[3] = {1, 2, 3};
    Span<int> mut(data);
    Span<const int> cs = mut;  // implicit conversion
    EXPECT_EQ(cs.size(), 3u);
    EXPECT_EQ(cs[1], 2);
}

// front()/back() return references, so writing through them mutates
// the underlying buffer (no copy is made).
TEST(Span, FrontAndBackReturnReferences) {
    int data[3] = {10, 20, 30};
    Span<int> s(data);
    s.front() = 100;
    s.back()  = 300;
    EXPECT_EQ(data[0], 100);
    EXPECT_EQ(data[2], 300);
    EXPECT_EQ(s[1], 20);
}

// first(k) and last(k) take a sub-view from each end without copying.
TEST(Span, FirstAndLast) {
    int data[5] = {1, 2, 3, 4, 5};
    Span<int> s(data);
    auto head = s.first(2);
    auto tail = s.last(2);
    ASSERT_EQ(head.size(), 2u);
    ASSERT_EQ(tail.size(), 2u);
    EXPECT_EQ(head[0], 1);
    EXPECT_EQ(head[1], 2);
    EXPECT_EQ(tail[0], 4);
    EXPECT_EQ(tail[1], 5);
}

// subspan(offset, count) gives an arbitrary middle slice, and the
// default `count` argument means "to the end".
TEST(Span, Subspan) {
    int data[6] = {0, 1, 2, 3, 4, 5};
    Span<int> s(data);
    auto mid = s.subspan(2, 3);
    ASSERT_EQ(mid.size(), 3u);
    EXPECT_EQ(mid[0], 2);
    EXPECT_EQ(mid[2], 4);

    // Default count: take everything from offset 4 onward.
    auto rest = s.subspan(4);
    ASSERT_EQ(rest.size(), 2u);
    EXPECT_EQ(rest[0], 4);
    EXPECT_EQ(rest[1], 5);
}

// Range-based for must yield the same elements as manual indexing.
// Confirms our begin()/end() pair is wired up correctly.
TEST(Span, RangeForMatchesIndexing) {
    int data[4] = {7, 8, 9, 10};
    Span<int> s(data);
    int idx = 0;
    for (int v : s) {
        EXPECT_EQ(v, s[static_cast<std::size_t>(idx++)]);
    }
    EXPECT_EQ(idx, 4);
}

// size_bytes() is `size() * sizeof(T)`, useful for memcpy-style code.
TEST(Span, SizeBytesEqualsSizeTimesSizeofT) {
    std::int32_t arr[3]{};
    Span<std::int32_t> s(arr);
    EXPECT_EQ(s.size_bytes(), 3u * sizeof(std::int32_t));
}

namespace {
// Compile-time smoke check: a default Span is empty in constexpr
// context. The static_assert triggers a build failure if the
// implementation regresses; nothing runs at test time.
static_assert(Span<int>{}.empty(), "default span must be empty in constexpr");
static_assert(Span<int>{}.size() == 0u, "default size must be zero");
}  // namespace
