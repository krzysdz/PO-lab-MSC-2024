#pragma once
#include <version>

// libc++ fully supports P1206R7 since 17.0.0, but defines only `__cpp_lib_ranges_to` until 19.0.0
#if _LIBCPP_VERSION >= 170000 && _LIBCPP_VERSION < 190000 && !defined(__cpp_lib_containers_ranges) && __cpp_lib_ranges_to_container >= 202202L
#define __cpp_lib_containers_ranges 202202L
#endif
