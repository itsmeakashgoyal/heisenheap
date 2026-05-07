# Convenience Makefile for building and testing heisenheap on the host.
# Wraps cmake / ctest -- no docker dependency. Run `make help` to see targets.

BUILD_DIR      ?= build/default
BUILD_TYPE     ?= Debug
JOBS           ?= $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
CMAKE          ?= cmake
CTEST          ?= ctest
CLANG_FORMAT   ?= clang-format
FORMAT_DIRS    ?= include src tests examples
FORMAT_GLOBS   := -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.cc'
CMAKE_FLAGS    ?=
TARGET         ?=
CTEST_TIMEOUT  ?= 60
EXAMPLE        ?= basic_usage
VERBOSE        ?= 0

# Surface FetchContent (e.g. googletest) clone progress so the configure step
# does not look frozen on first run. Set FETCHCONTENT_QUIET=ON to suppress.
FETCHCONTENT_QUIET ?= OFF
CONFIGURE_FLAGS := -DFETCHCONTENT_QUIET=$(FETCHCONTENT_QUIET) $(CMAKE_FLAGS)
BUILD_VERBOSE   := $(if $(filter-out 0,$(VERBOSE)),--verbose,)

EXAMPLES := basic_usage

.DEFAULT_GOAL := help

.PHONY: help configure build test examples run debug release \
        clean distclean asan tsan rebuild format format-check

help: ## Show this help message
	@cat $(MAKEFILE_LIST) | grep -E '^[a-zA-Z_-]+:.*?## .*$$' | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

$(BUILD_DIR)/CMakeCache.txt:
	@echo ">>> Configuring (first-run downloads GoogleTest -- progress streams below)"
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CONFIGURE_FLAGS)

configure: $(BUILD_DIR)/CMakeCache.txt ## Configure the CMake build directory (set VERBOSE=1 for verbose output)

build: configure ## Build the library (override TARGET=<name>; set VERBOSE=1 to see compiler commands)
	$(CMAKE) --build $(BUILD_DIR) -j $(JOBS) $(BUILD_VERBOSE) $(if $(TARGET),--target $(TARGET))
	@echo
	@echo "Build finished. Artifacts are in $(CURDIR)/$(BUILD_DIR)"

test: build ## Run all tests via ctest
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure --timeout $(CTEST_TIMEOUT)

examples: configure ## Build all example binaries
	@for ex in $(EXAMPLES); do \
		$(CMAKE) --build $(BUILD_DIR) -j $(JOBS) --target $$ex || exit 1; \
	done
	@echo
	@echo "Examples built in $(CURDIR)/$(BUILD_DIR)/examples"

run: build ## Build and run an example (override EXAMPLE=<name>, default: basic_usage)
	@if [ ! -x "$(BUILD_DIR)/examples/$(EXAMPLE)" ]; then \
		$(CMAKE) --build $(BUILD_DIR) -j $(JOBS) --target $(EXAMPLE); \
	fi
	./$(BUILD_DIR)/examples/$(EXAMPLE)

debug: ## Build in Debug mode (default)
	$(MAKE) build BUILD_TYPE=Debug

release: ## Build in Release mode
	$(MAKE) build BUILD_TYPE=Release BUILD_DIR=build/release

rebuild: clean build ## Clean everything and rebuild from scratch

clean: ## Remove ALL build directories
	rm -rf build

distclean: clean ## clean + remove generated caches and compile_commands.json
	rm -rf compile_commands.json .cache

# Sanitizer builds. Each one builds into a separate build/<name> directory
# so they coexist with the default build.
asan: ## Build + test with AddressSanitizer
	$(MAKE) build BUILD_DIR=build/asan BUILD_TYPE=Debug \
		CMAKE_FLAGS="-DHH_ASAN=ON $(CMAKE_FLAGS)"
	$(CTEST) --test-dir build/asan --output-on-failure --timeout $(CTEST_TIMEOUT)

tsan: ## Build + test with ThreadSanitizer
	$(MAKE) build BUILD_DIR=build/tsan BUILD_TYPE=Debug \
		CMAKE_FLAGS="-DHH_TSAN=ON $(CMAKE_FLAGS)"
	$(CTEST) --test-dir build/tsan --output-on-failure --timeout $(CTEST_TIMEOUT)

format: ## Apply .clang-format to all C/C++ sources in $(FORMAT_DIRS)
	@find $(FORMAT_DIRS) -type f \( $(FORMAT_GLOBS) \) -print0 \
		| xargs -0 $(CLANG_FORMAT) -i --style=file
	@echo "Formatted sources under: $(FORMAT_DIRS)"

format-check: ## Verify all C/C++ sources match .clang-format (non-zero on drift)
	@find $(FORMAT_DIRS) -type f \( $(FORMAT_GLOBS) \) -print0 \
		| xargs -0 $(CLANG_FORMAT) --dry-run --Werror --style=file
