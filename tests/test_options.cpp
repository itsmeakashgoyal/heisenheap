// Tests for include/heisenheap/options.h.
//
// We test two contracts:
//   1. OptionsBuilder produces an Options whose fields exactly match
//      the chained setter calls.
//   2. Options::validate() returns std::nullopt for legal configurations
//      and a populated Error for each enumerated illegal one.
#include <gtest/gtest.h>

#include "heisenheap/options.h"

using heisenheap::ErrorCode;
using heisenheap::Options;
using heisenheap::OptionsBuilder;

// Default-constructed Options should pass validation — the in-source
// defaults are meant to be a sensible production configuration.
TEST(Options, DefaultsValidate) {
    const Options o{};
    const auto    err = o.validate();
    EXPECT_FALSE(err.has_value()) << "default Options should be valid";
}

// sample_rate_ < 1 is impossible: "sample 1 in 0" makes no sense.
// validate() must reject it with InvalidOptions.
TEST(Options, SampleRateZeroFailsValidation) {
    const auto o   = OptionsBuilder{}.sample_rate(0).build();
    const auto err = o.validate();
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(err->code_, ErrorCode::InvalidOptions);
}

// slot_pages_ must be a power of two for slot strides to align nicely.
// 3 is a non-power-of-two; validate() must reject it.
TEST(Options, SlotPagesNotPowerOfTwoFails) {
    const auto o   = OptionsBuilder{}.slot_pages(3).build();
    const auto err = o.validate();
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(err->code_, ErrorCode::InvalidOptions);
}

// slot_pages_ has an upper bound of 64 (kSlotPagesMax in options.cpp).
// 128 must be rejected to prevent huge accidental VM reservations.
TEST(Options, SlotPagesAboveMaxFails) {
    const auto o   = OptionsBuilder{}.slot_pages(128).build();
    const auto err = o.validate();
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(err->code_, ErrorCode::InvalidOptions);
}

// quarantine_capacity_ cannot exceed slot_count_ — there are not enough
// slots to fill that many quarantine entries.
TEST(Options, QuarantineLargerThanSlotCountFails) {
    const auto o   = OptionsBuilder{}.slot_count(8).quarantine_capacity(32).build();
    const auto err = o.validate();
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(err->code_, ErrorCode::InvalidOptions);
}

// max_alloc_size_ == 0 would route every request to "not sampled"
// because no allocation is below 0 bytes.
TEST(Options, MaxAllocSizeZeroFails) {
    const auto o   = OptionsBuilder{}.max_alloc_size(0).build();
    const auto err = o.validate();
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(err->code_, ErrorCode::InvalidOptions);
}

// End-to-end builder smoke test: every fluent setter, in one chain, and
// every resulting field is read back to confirm the intended value
// reached its slot. Catches accidental copy-paste regressions in the
// setter list.
TEST(Options, BuilderChainingProducesExpectedValues) {
    const auto o = OptionsBuilder{}
                       .sample_rate(500)
                       .slot_count(128)
                       .slot_pages(2)
                       .max_alloc_size(std::size_t{4} << 20)
                       .large_capacity(64)
                       .history_capacity(512)
                       .quarantine_capacity(32)
                       .enable_metrics(false)
                       .enable_logging(true)
                       .build();
    EXPECT_EQ(o.sample_rate_, 500u);
    EXPECT_EQ(o.slot_count_, 128u);
    EXPECT_EQ(o.slot_pages_, 2u);
    EXPECT_EQ(o.max_alloc_size_, std::size_t{4} << 20);
    EXPECT_EQ(o.large_capacity_, 64u);
    EXPECT_EQ(o.history_capacity_, 512u);
    EXPECT_EQ(o.quarantine_capacity_, 32u);
    EXPECT_FALSE(o.enable_metrics_);
    EXPECT_TRUE(o.enable_logging_);
}

// constexpr-usability is part of the builder's contract — handy when
// tests want to assemble an Options at compile time inside a
// static_assert.
TEST(Options, BuilderIsConstexpr) {
    constexpr auto o = OptionsBuilder{}.sample_rate(7).build();
    static_assert(o.sample_rate_ == 7u, "OptionsBuilder must be constexpr-usable");
    EXPECT_EQ(o.sample_rate_, 7u);
}

// large_capacity_ == 0 is a legitimate configuration: it disables the
// large allocator. validate() must let it through.
TEST(Options, LargeCapacityZeroIsValid) {
    const auto o = OptionsBuilder{}.large_capacity(0).build();
    EXPECT_FALSE(o.validate().has_value());
}

// history_capacity_ == 0 is also legitimate: it disables history
// retention. validate() must let it through.
TEST(Options, HistoryCapacityZeroIsValid) {
    const auto o = OptionsBuilder{}.history_capacity(0).build();
    EXPECT_FALSE(o.validate().has_value());
}
