#pragma once
#include <algorithm>
#include <cmath>
#include <concepts>
#include <iostream>
#include <iterator>
#include <ranges>

template <std::floating_point F, F tolerance = F{}> constexpr bool floating_eq(F a, F b)
{
    return std::fabs(a - b) <= tolerance;
}

template <std::input_iterator It1, std::input_iterator It2,
          std::iter_value_t<It1> tolerance = std::iter_value_t<It1>{}>
constexpr bool compare_tolerance(It1 first1, It1 last1, It2 first2, It2 last2)
{
    return std::equal(first1, last1, first2, last2, floating_eq<std::iter_value_t<It1>, tolerance>);
}

template <std::ranges::input_range R1, std::ranges::input_range R2,
          std::ranges::range_value_t<R1> tolerance = std::ranges::range_value_t<R1>{}>
constexpr bool compare_tolerance(R1 &&r1, R2 &&r2)
{
    return std::ranges::equal(r1, r2, floating_eq<std::ranges::range_value_t<R1>, tolerance>);
}

template <std::invocable F> void it_should_not_throw(F lambda)
{
    try {
        lambda();
        std::cerr << "SUCCESS\n";
    } catch (const std::exception &e) {
        std::cerr << "FAILED with error:\n\t" << e.what() << '\n';
    } catch (...) {
        std::cerr << "FAILED\n";
    }
}
