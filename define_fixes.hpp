/// @file define_fixes.hpp
/// @brief C++ standard library checks and workarounds.

#pragma once
#include <version>

// libc++ fully supports P1206R7 since 17.0.0, but defines only `__cpp_lib_ranges_to` until 19.0.0
#if _LIBCPP_VERSION >= 170000 && _LIBCPP_VERSION < 190000 && !defined(__cpp_lib_containers_ranges) \
    && __cpp_lib_ranges_to_container >= 202202L
#define __cpp_lib_containers_ranges 202202L
#endif

#if defined(_MSC_VER) && !(__cpp_lib_constexpr_cmath >= 202202L || __cpp_constexpr >= 202207L)
#error                                                                                             \
    "MSVC with support for P2448R2 (https://wg21.link/p2448r2) or P0533R9 (https://wg21.link/p0533R9) is required."
#endif
