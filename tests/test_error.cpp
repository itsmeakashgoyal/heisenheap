// Tests for include/heisenheap/error.h.
//
// We verify three contracts of the Error / ErrorCode / make_error trio:
//   1. make_error fills code_ + message_ correctly
//   2. The default-constructed Error is the "no error" sentinel
//   3. describe() returns a non-null string for every enumerator
#include <cstring>

#include <gtest/gtest.h>

#include "heisenheap/error.h"

using heisenheap::describe;
using heisenheap::Error;
using heisenheap::ErrorCode;
using heisenheap::make_error;

// make_error(code) should set the code field and look the message up
// from describe(). The message must be non-null and non-empty.
TEST(Error, MakeErrorCarriesCodeAndMessage) {
    const auto e = make_error(ErrorCode::PoolExhausted);
    EXPECT_EQ(e.code_, ErrorCode::PoolExhausted);
    ASSERT_NE(e.message_, nullptr);
    EXPECT_GT(std::strlen(e.message_), 0u);
}

// A value-initialised Error is the "no error" sentinel: code_ == None
// and ok() returns true. The implicit-bool conversion (used in
// `if (error) { ... }`) reports false because the value isn't an error.
TEST(Error, ValueInitialisedIsNoneAndOk) {
    Error e{};
    EXPECT_EQ(e.code_, ErrorCode::None);
    EXPECT_TRUE(e.ok());
    EXPECT_FALSE(static_cast<bool>(e));
}

// The implicit-bool conversion flips for any non-None Error so that
// callers can write `if (auto err = something(); err) { ... }`.
TEST(Error, NonNoneIsTruthy) {
    const Error e = make_error(ErrorCode::NotOwned);
    EXPECT_FALSE(e.ok());
    EXPECT_TRUE(static_cast<bool>(e));
}

// describe() must answer for every enumerator. If we add a new code in
// the header but forget to add it to the switch in error.cpp, the
// compiler would warn about an unhandled case (-Werror would fail the
// build), so this loop is effectively a runtime double-check.
TEST(Error, DescribeReturnsNonNullForEveryEnumerator) {
    const ErrorCode all[] = {
        ErrorCode::None,
        ErrorCode::Uninitialized,
        ErrorCode::AlreadyInitialized,
        ErrorCode::InvalidOptions,
        ErrorCode::SizeZero,
        ErrorCode::SizeAboveMaximum,
        ErrorCode::AlignmentNotPowerOfTwo,
        ErrorCode::AlignmentTooLarge,
        ErrorCode::NotSampled,
        ErrorCode::NotOwned,
        ErrorCode::DoubleFree,
        ErrorCode::InvalidFree,
        ErrorCode::PoolExhausted,
        ErrorCode::LargeCapacityExhausted,
        ErrorCode::OutOfMemory,
        ErrorCode::DisabledOnCurrentThread,
        ErrorCode::Internal,
    };
    for (auto c : all) {
        const char* d = describe(c);
        ASSERT_NE(d, nullptr);
        EXPECT_GT(std::strlen(d), 0u);
    }
}

// Passing HH_HERE through to make_error must surface the location in
// the resulting Error::where_ field. This is the path that real error
// sites use to record "I detected this bug at file X, line Y".
TEST(Error, MakeErrorPropagatesSourceLocation) {
    const auto e = make_error(ErrorCode::InvalidFree, HH_HERE);
    EXPECT_EQ(e.code_, ErrorCode::InvalidFree);
    EXPECT_GT(e.where_.line_, 0u);
    ASSERT_NE(e.where_.file_, nullptr);
    EXPECT_GT(std::strlen(e.where_.file_), 0u);
}
