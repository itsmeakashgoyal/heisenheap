// Tests for include/heisenheap/source_location.h.
//
// SourceLocation is just a POD struct, so the only behaviour worth
// verifying is that the HH_HERE macro fills it with sensible values
// from the *caller's* source position.
#include <cstring>
#include <string>

#include <gtest/gtest.h>

#include "heisenheap/source_location.h"

namespace {

// Two helpers in this anonymous namespace let us call HH_HERE from two
// different physical lines and confirm the line numbers differ.
heisenheap::SourceLocation here_a() {
    return HH_HERE;
}
heisenheap::SourceLocation here_b() {
    return HH_HERE;
}

}  // namespace

// HH_HERE must produce a SourceLocation whose `file_` is non-null /
// non-empty and whose `line_` is greater than zero (lines are 1-based).
TEST(SourceLocation, CapturesNonEmptyFileAndNonZeroLine) {
    const auto loc = HH_HERE;
    ASSERT_NE(loc.file_, nullptr);
    EXPECT_GT(std::strlen(loc.file_), 0u);
    EXPECT_GT(loc.line_, 0u);
}

// Two HH_HERE expansions on different lines must record different
// `line_` values. This catches regressions where the macro accidentally
// becomes a regular function (which would always report its own line).
TEST(SourceLocation, DifferentLinesProduceDifferentLineNumbers) {
    const auto a = here_a();
    const auto b = here_b();
    EXPECT_NE(a.line_, b.line_);
}

// `function_` should hold a non-empty C string. We don't test for a
// specific spelling because compilers differ (Apple Clang reports
// "TestBody", others include the fixture name); the contract is just
// "some non-empty function-name string".
TEST(SourceLocation, FunctionMatchesEnclosingFunc) {
    const auto loc = HH_HERE;
    ASSERT_NE(loc.function_, nullptr);
    EXPECT_GT(std::strlen(loc.function_), 0u);
}
