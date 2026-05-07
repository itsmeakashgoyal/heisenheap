// Inspired by: GWP-ASan's Options struct. Simplified by: a Builder
// pattern with constexpr fluent setters; validate() returns
// std::optional<Error> instead of throwing.
//
// =============================================================================
// What this header is for
// =============================================================================
//
// Heisenheap is configured exactly once, at startup, by passing an
// `Options` value to `Heisenheap::create()`. Everything that's tunable —
// sample rate, slot count, slot size, history retention, capacity caps —
// lives in this struct.
//
// To set up Options, callers use the fluent OptionsBuilder:
//
//     auto opts = heisenheap::OptionsBuilder{}
//                     .sample_rate(500)
//                     .slot_count(128)
//                     .build();
//
// The builder pattern is preferred over a 9-argument constructor or
// positional aggregate init because:
//   * each `.field(value)` call self-documents what's being set
//   * defaults stay in the struct, not duplicated at every call site
//   * adding a new tunable doesn't break existing callers
//
// `Options::validate()` returns `std::optional<Error>` — `nullopt`
// means "configuration is valid", a populated Error explains the first
// problem found. `Heisenheap::create()` calls validate() before doing
// anything irreversible.
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "heisenheap/error.h"

namespace heisenheap {

/// Configuration for one Heisenheap instance.
///
/// All fields are public so the struct is trivially aggregate-initialisable
/// and inspectable. The trailing underscore on every field is the project
/// style rule (see STYLE.md).
///
/// Default values are tuned for a small reasonable production setup
/// (sample 1-in-1000, 64 guarded slots of 1 page each). Override via
/// `OptionsBuilder` only when you need different behaviour.
struct Options {
    // -------------------------------------------------------------------
    // Sampling
    // -------------------------------------------------------------------

    /// On average, every Nth allocation is sampled (= sent through the
    /// guarded path). 1 means "guard every allocation" (only useful in
    /// tests — too expensive in production). Must be >= 1.
    std::uint32_t sample_rate_ = 1000;

    // -------------------------------------------------------------------
    // Guarded slot pool sizing
    // -------------------------------------------------------------------

    /// Number of guarded slots in the pool. Each slot can hold one live
    /// allocation. Larger pool = more concurrent guarded allocs survive
    /// without hitting the quarantine eviction path.
    std::uint32_t slot_count_ = 64;

    /// Number of payload pages per slot, MUST be a power of two and
    /// MUST be <= 64. Determines the largest allocation the pool can
    /// hold (slot_pages_ * OS page size). Larger allocations route to
    /// the LargeAllocator instead.
    std::uint32_t slot_pages_ = 1;

    // -------------------------------------------------------------------
    // Routing thresholds
    // -------------------------------------------------------------------

    /// Largest size the allocator will sample. Anything bigger is
    /// passed straight through to the system allocator (NotSampled).
    /// Default: 1 MiB.
    std::size_t max_alloc_size_ = std::size_t{1} << 20;

    // -------------------------------------------------------------------
    // Large-allocation backend
    // -------------------------------------------------------------------

    /// Maximum number of simultaneously-live "large" allocations
    /// (those that overflow the slot pool). Each large alloc occupies
    /// its own VM reservation. 0 disables the large allocator entirely
    /// — over-sized requests then fall through to the system allocator.
    std::uint32_t large_capacity_ = 32;

    // -------------------------------------------------------------------
    // History retention
    // -------------------------------------------------------------------

    /// Capacity of the ring of "recently freed" records. The crash
    /// handler walks this ring to attribute use-after-free crashes
    /// even when the slot has already been recycled. 0 disables
    /// history.
    std::uint32_t history_capacity_ = 256;

    // -------------------------------------------------------------------
    // Quarantine
    // -------------------------------------------------------------------

    /// Number of freed slots held in quarantine before being recycled.
    /// Larger quarantine = better use-after-free coverage but more
    /// committed VM. MUST be <= slot_count_.
    std::uint32_t quarantine_capacity_ = 16;

    // -------------------------------------------------------------------
    // Observability
    // -------------------------------------------------------------------

    /// If true, heisenheap maintains atomic counters for sampled /
    /// declined / pool / large / fault events (cheap, on by default).
    bool enable_metrics_ = true;

    /// If true, lifecycle events and faults are written to the logger.
    /// Off by default because the logger is meant for debugging, not
    /// production.
    bool enable_logging_ = false;

    /// Validates every field of this Options instance. Returns
    /// `std::nullopt` if all fields are within range; otherwise returns
    /// an `Error` describing the first violation.
    ///
    /// Implementation lives in src/options.cpp.
    [[nodiscard]] std::optional<Error> validate() const noexcept;
};

/// Fluent builder for `Options`.
///
/// Each setter returns `*this` so calls can be chained. All setters and
/// `build()` are `constexpr`, so an Options value can be assembled at
/// compile time and used inside `static_assert` (handy in tests).
///
///     constexpr auto opts = OptionsBuilder{}.sample_rate(500).build();
class OptionsBuilder {
public:
    /// Default-construct the builder with the same defaults that
    /// `Options{}` would produce.
    constexpr OptionsBuilder() noexcept = default;

    // ---- Fluent setters. One per Options field. -----------------------
    //
    // Each one is a single-line `constexpr` that updates the underlying
    // Options and returns *this so the next setter can be chained.

    constexpr OptionsBuilder& sample_rate(std::uint32_t v) noexcept {
        options_.sample_rate_ = v;
        return *this;
    }
    constexpr OptionsBuilder& slot_count(std::uint32_t v) noexcept {
        options_.slot_count_ = v;
        return *this;
    }
    constexpr OptionsBuilder& slot_pages(std::uint32_t v) noexcept {
        options_.slot_pages_ = v;
        return *this;
    }
    constexpr OptionsBuilder& max_alloc_size(std::size_t v) noexcept {
        options_.max_alloc_size_ = v;
        return *this;
    }
    constexpr OptionsBuilder& large_capacity(std::uint32_t v) noexcept {
        options_.large_capacity_ = v;
        return *this;
    }
    constexpr OptionsBuilder& history_capacity(std::uint32_t v) noexcept {
        options_.history_capacity_ = v;
        return *this;
    }
    constexpr OptionsBuilder& quarantine_capacity(std::uint32_t v) noexcept {
        options_.quarantine_capacity_ = v;
        return *this;
    }
    constexpr OptionsBuilder& enable_metrics(bool v) noexcept {
        options_.enable_metrics_ = v;
        return *this;
    }
    constexpr OptionsBuilder& enable_logging(bool v) noexcept {
        options_.enable_logging_ = v;
        return *this;
    }

    /// Returns the assembled Options value. The builder itself is not
    /// modified, so it can be reused or chained further.
    [[nodiscard]] constexpr Options build() const noexcept {
        return options_;
    }

private:
    Options options_{};  // running configuration; modified by setters
};

}  // namespace heisenheap
