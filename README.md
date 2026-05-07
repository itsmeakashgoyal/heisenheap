# Heisenheap

> *The heap bug you never see is the one that hits production.*

A learning-oriented, from-scratch **probabilistic guarded heap allocator**
in modern C++17. Most allocations pass through invisibly; a small
randomly-sampled subset gets routed through a guarded path with
`mprotect`-backed guard pages on either side of the payload. When user
code overruns a buffer or touches memory after free, the access hits a
guard page, the OS raises `SIGSEGV`, and a signal handler attributes the
fault to the offending allocation — with the allocation and free stack
traces intact.

The name is a nod to Heisenberg: any given allocation lives in
*superposition* between "ordinary `malloc`" and "guarded slot" until
sampling collapses it into one or the other. The bugs you observe are
the ones that got sampled.

## Status

Project under active development. Built on macOS (Apple
Silicon); designed to be portable to Linux. The Platform Abstraction
Layer is in place so a Windows backend can be added without touching
call sites — no Windows backend ships today.

## Public references / inspiration

This is a clean-room implementation built by reading public
documentation about probabilistic guarded allocators.

- LLVM **GWP-ASan** — <https://llvm.org/docs/GwpAsan.html>
- Chromium **GWP-ASan / PHC** (BSD-3) — <https://chromium.googlesource.com/chromium/src/+/main/components/gwp_asan/>
- **Electric Fence** (1987) by Bruce Perens — original `mprotect`
  guard-page allocator
- Apple `<malloc/malloc.h>` — public macOS malloc-zone API
- POSIX `mmap`, `mprotect`, `sigaction` specifications

## Build

Requires CMake ≥ 3.24 and a C++17 compiler.

```bash
make build       # configure + build (Debug)
make test        # build + run hh_tests
make run         # build + run examples/basic_usage
make asan        # build + test with AddressSanitizer
make help        # list all targets
```

Or directly with CMake:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

## Project layout

```
include/heisenheap/   public headers (flat) + pal/ (platform abstraction)
src/                  implementation
src/platform/         per-platform PAL implementations (selected by CMake)
tests/                GoogleTest suite (single binary: hh_tests)
examples/             small standalone usage examples
cmake/                compiler-flag, sanitizer, and platform-source modules
```

## Style

Heisenheap follows the **Google C++ Style Guide** with a small set of
project-specific deltas.

## License

MIT — see `LICENSE`.
