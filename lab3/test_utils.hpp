#pragma once
#include "../lab1/util.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <concepts>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <ranges>
#include <string>
#include <string_view>

using namespace std::string_view_literals;

template <typename Path>
std::map<int, double> read_test_data(const Path &filename, std::size_t skiplines = 0)
{
    std::map<int, double> result;
    std::string line;
    std::ifstream data_file{ filename };
    for (std::size_t i = 0; i < skiplines; ++i)
        data_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    while (std::getline(data_file, line)) {
        int x;
        double y;
        const auto comma_pos = line.find(',');
        // It's just test code, let's ignore possible errors
        std::from_chars(line.data(), line.data() + comma_pos, x);
#if defined(__GLIBCXX__) || defined(_MSVC_STL_UPDATE)
        std::from_chars(line.data() + comma_pos + 1, line.data() + line.size(), y);
#else // libc++ does not support floating-point std::from_chars
        const auto y_str = line.substr(comma_pos + 1);
        y = std::stod(y_str);
#endif
        result.try_emplace(x, y);
    }
    return result;
}

template <std::floating_point F, F tolerance = F{}> constexpr bool floating_eq(F a, F b)
{
    return std::fabs(a - b) <= tolerance;
}

template <std::invocable F> void it_should_not_throw(std::string_view test_name, F test)
{
    std::cerr << test_name << ": ";
    try {
        std::invoke(test);
        std::cerr << "SUCCESS\n";
    } catch (const std::exception &e) {
        std::cerr << "FAILED with error:\n\t" << e.what() << '\n';
    } catch (...) {
        std::cerr << "FAILED\n";
    }
}

template <typename E = std::exception>
void it_should_throw(std::string_view test_name, std::invocable auto test,
                     std::optional<std::string_view> what = std::nullopt)
{
    const auto log_different = [&what] {
        std::cerr << "FAILED, exception of a different type was thrown:\n\tExpected "
                  << typeid(E).name();
        if constexpr (std::derived_from<E, std::exception>)
            if (what)
                std::cerr << " with reason: " << what.value();
    };
    std::cerr << test_name << ": ";
    try {
        std::invoke(test);
        std::cerr << "FAILED, no exception was thrown, " << typeid(E).name() << " expected\n";
    } catch (const E &e) {
        if constexpr (std::derived_from<E, std::exception>) {
            if (!what || what.value() == e.what())
                std::cerr << "SUCCESS\n";
            else
                std::cerr << "FAILED\n\tExpected reason: " << what.value()
                          << "\n\te.what() returned: " << e.what() << '\n';
            return;
        }
        std::cerr << "SUCCESS\n";
// If E is std::exception compilers will generate a warning that this catch will not be used
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexceptions"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexceptions"
    } catch (const std::exception &e) {
#pragma clang diagnostic pop
#pragma GCC diagnostic pop
        log_different();
        std::cerr << "\n\tCaught " << typeid(e).name() << " with reason: " << e.what() << '\n';
    } catch (...) {
        log_different();
        std::cerr << "\n\tCaught unexpected exception not derived from std::exception\n";
    }
}

template <std::invocable F, typename T, typename Pred = std::ranges::equal_to>
void it_should_return(std::string_view test_name, const T &value, F test, Pred pred = {})
    requires DirectlyComparable<T, std::invoke_result_t<F>, Pred>
    || RangeComparable<T, std::invoke_result_t<F>, Pred>
{
    std::cerr << test_name << ": ";
    auto result = std::invoke(test);
    using R = decltype(result);
    bool matches = false;
    if constexpr (std::ranges::input_range<T> && std::ranges::input_range<R>)
        matches = std::ranges::equal(value, result, pred);
    else
        matches = std::invoke(pred, value, result);
    if (matches)
        std::cerr << "SUCCESS\n";
    else {
#if __cpp_lib_format_ranges >= 202207L
        std::cerr << std::format("FAILED\n\tExpected: {}\n\tReturned: {}\n", value, result);
#else
        if constexpr (Streamable<T> && Streamable<R>)
            std::cerr << "FAILED\n\tExpected: " << value << "\n\tReturned: " << result << '\n';
        else if constexpr (InputRangeOver<T, []<Streamable> {}>
                           && InputRangeOver<R, []<Streamable> {}>) {
            // result and expected value can be iterated over and the values can be printed using
            // stream operators: https://godbolt.org/z/obPMK8Yn3
            std::cerr << "FAILED\n\tExpected: [";
            const char *sep = "";
            for (const auto &v : value) {
                std::cerr << sep << v;
                sep = ", ";
            }
            std::cerr << "]\n\tReturned: [";
            sep = "";
            for (const auto &v : result) {
                std::cerr << sep << v;
                sep = ", ";
            }
            std::cerr << "]\n";
        } else
            std::cerr << "FAILED\n";
#endif
    }
}
