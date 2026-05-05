#pragma once

// HH_API marks symbols that should be visible outside the shared library.
// When building as a static library (the default) this expands to nothing.
#if defined(HH_BUILDING_SHARED) || defined(HH_SHARED)
    #define HH_API __attribute__((visibility("default")))
#else
    #define HH_API
#endif
