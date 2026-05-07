// Inspired by: std::span (C++20). Simplified by: dynamic extent only,
// no static-extent overload set, no as_bytes() helpers (call sites
// construct Span<std::byte> directly when they want byte access).
//
// =============================================================================
// What this header is for
// =============================================================================
//
// All over the codebase we need to talk about "a contiguous sequence of N
// elements starting at address P". The bare-bones way to do this in C++ is
// to pass a `(T* ptr, std::size_t size)` pair, like the C library does:
//
//     void process(const std::byte* data, std::size_t size);
//
// That works but the compiler doesn't link the pointer and size — a caller
// can pass mismatched values, the size can be forgotten, and `nullptr`
// with size 5 silently means "5 elements at address 0". A `Span<T>` bundles
// the two together and ensures iteration / indexing / sub-views are
// type-safe and consistent.
//
// `std::span` is the standard library's version of this idea, but it
// landed in C++20. We target C++17, so we ship a small in-house Span<T>
// covering ~80% of std::span's surface. If we ever bump to C++20, the
// migration is a one-pass `s/heisenheap::Span/std::span/`.
//
// Important: a Span is NON-OWNING. It doesn't allocate, doesn't free, and
// doesn't extend the lifetime of whatever buffer it points at. The caller
// is responsible for making sure the underlying memory outlives every
// Span that views it.
// =============================================================================
#pragma once

#include <cassert>      // for assert() inside operator[] / first / last
#include <cstddef>      // std::size_t, std::ptrdiff_t
#include <type_traits>  // std::remove_cv_t, std::is_convertible

namespace heisenheap {

/// A non-owning view over a contiguous sequence of `T` elements.
///
/// Span<T> is essentially `(T* data_, std::size_t size_)` with safe
/// iteration, indexing, and sub-view operations. Because it doesn't
/// own anything, you can copy it as cheaply as a struct of two pointers.
///
/// Typical uses inside heisenheap:
///   * `Span<std::byte> Allocation::bytes()` — view of a guarded payload
///   * `pal::stack_walk::capture(Span<void*> out)` — output buffer for frames
///   * function arguments that previously took `(T*, size_t)` pairs
///
/// @tparam T element type. Use `Span<const T>` for read-only views.
template <typename T>
class Span {
public:
    // ---- Standard-library-style nested type aliases. -----------------
    // These let generic code that knows about std::span's protocol also
    // work with our Span (e.g. `typename SpanLike::value_type`).
    using element_type    = T;
    using value_type      = std::remove_cv_t<T>;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;
    using iterator        = T*;  // raw pointer iterators are enough for our purposes
    using const_iterator  = const T*;

    /// Construct an empty span (`size_ == 0`, `data_ == nullptr`).
    /// Equivalent to a default-initialised pointer + size pair.
    constexpr Span() noexcept = default;

    /// Construct from a pointer and a count.
    ///
    /// @param data First element of the sequence. May be null only if
    ///             `count` is also zero.
    /// @param count Number of elements `data` points at. The caller
    ///             promises `data[0..count-1]` are valid memory.
    constexpr Span(pointer data, size_type count) noexcept : data_(data), size_(count) {}

    /// Construct from a built-in array — the size is deduced from the
    /// array's type, eliminating an opportunity for a (ptr,size) mismatch.
    ///
    ///     int arr[7];
    ///     Span<int> s(arr);      // s.size() == 7
    template <std::size_t N>
    constexpr Span(T (&arr)[N]) noexcept : data_(arr), size_(N) {}

    /// Implicit conversion `Span<U>` → `Span<T>` whenever `U(*)[]` is
    /// convertible to `T(*)[]`. In practice, this enables the common
    /// case of converting a mutable view to a const view:
    ///
    ///     Span<int>       writable = ...;
    ///     Span<const int> readonly = writable;   // OK, no .as_const() call
    ///
    /// The pointer-to-array trick is the same one std::span uses; it
    /// allows `int* → const int*` but rejects unrelated unsafe casts.
    template <typename U, typename = std::enable_if_t<std::is_convertible<U (*)[], T (*)[]>::value>>
    constexpr Span(const Span<U>& other) noexcept : data_(other.data()), size_(other.size()) {}

    // -------- Capacity and raw access. ----------------------------------

    /// Pointer to the first element (or nullptr for an empty span).
    [[nodiscard]] constexpr pointer data() const noexcept {
        return data_;
    }

    /// Number of elements in the view.
    [[nodiscard]] constexpr size_type size() const noexcept {
        return size_;
    }

    /// Number of *bytes* the view spans, i.e. `size() * sizeof(T)`.
    [[nodiscard]] constexpr size_type size_bytes() const noexcept {
        return size_ * sizeof(element_type);
    }

    /// True if the span contains zero elements.
    [[nodiscard]] constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    // -------- Element access. -------------------------------------------
    //
    // operator[] / front() / back() are *unchecked* in release builds
    // (matching std::span behaviour). The assert() calls catch
    // out-of-range access in debug builds. Use first()/last()/subspan()
    // when you want a bounds-checked sub-view.

    /// Returns a reference to the i-th element. UB if `i >= size()`.
    constexpr reference operator[](size_type i) const noexcept {
        assert(i < size_ && "heisenheap::Span::operator[]: index out of range");
        return data_[i];
    }

    /// Returns a reference to the first element. UB if the span is empty.
    constexpr reference front() const noexcept {
        assert(!empty() && "heisenheap::Span::front: empty span");
        return data_[0];
    }

    /// Returns a reference to the last element. UB if the span is empty.
    constexpr reference back() const noexcept {
        assert(!empty() && "heisenheap::Span::back: empty span");
        return data_[size_ - 1];
    }

    // -------- Iteration. ------------------------------------------------
    //
    // Raw pointers are valid C++ iterators when `T` is contiguous in
    // memory, which it is for any Span. This is enough to enable
    // `for (auto& x : span)` and the standard `<algorithm>` family.

    constexpr iterator begin() const noexcept {
        return data_;
    }
    constexpr iterator end() const noexcept {
        return data_ + size_;
    }

    // -------- Sub-views. ------------------------------------------------

    /// A view over the first `n` elements. UB if `n > size()`.
    [[nodiscard]] constexpr Span first(size_type n) const noexcept {
        assert(n <= size_);
        return Span(data_, n);
    }

    /// A view over the last `n` elements. UB if `n > size()`.
    [[nodiscard]] constexpr Span last(size_type n) const noexcept {
        assert(n <= size_);
        return Span(data_ + (size_ - n), n);
    }

    /// A view starting at `offset` and containing `count` elements
    /// (default: all remaining elements after `offset`).
    ///
    /// `count == size_type(-1)` is the sentinel value meaning "to the end",
    /// matching std::span's `dynamic_extent` overload.
    [[nodiscard]] constexpr Span subspan(
        size_type offset, size_type count = static_cast<size_type>(-1)) const noexcept {
        assert(offset <= size_);
        const size_type effective_count =
            (count == static_cast<size_type>(-1)) ? (size_ - offset) : count;
        assert(effective_count <= size_ - offset);
        return Span(data_ + offset, effective_count);
    }

private:
    // Trailing underscores: project style rule (see STYLE.md).
    pointer   data_ = nullptr;  // first element, or nullptr when empty
    size_type size_ = 0;        // element count
};

}  // namespace heisenheap
