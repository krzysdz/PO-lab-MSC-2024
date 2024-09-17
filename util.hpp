/// @file util.hpp
/// @brief A collection of helper functions and concepts. Often used for serialization.

#pragma once
#include "define_fixes.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <functional>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

/// @brief The concept is satisfied iff `T` is [trivially
/// copyable](https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable).
template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

/// @brief The concept is satisfied iff `T` is an [arithmetic
/// type](https://timsong-cpp.github.io/cppwp/n4950/basic.fundamental#13), that is integral or
/// floating-point.
template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

/// @brief The concept is satisfied if `B` can be used to access raw memory of other objects.
///
/// Satisfied only for `unsigned char` and `std::byte`.
template <typename B>
concept ByteRepr = std::same_as<B, unsigned char> || std::same_as<B, std::byte>;

/// The concept is satisfied only if `operator<<(std::ostream &, const T&)` exists.
template <typename T>
concept Streamable = requires(std::ostream &os, const T val) { os << val; };

/// @brief Satisfied if `T` satisfies `C`.
///
/// Based on
/// [Ed Catmur, _Higher-Order Template Metaprogramming with
/// C++23_](https://www.youtube.com/watch?v=KENynEQoqCo).
template <typename T, auto C>
concept Satisfies = requires { C.template operator()<T>(); };

/// @brief Satisfied if `T` is an
/// [`input_range`](https://eel.is/c++draft/range.req#concept:input_range) over values which satisfy
/// `C`.
///
/// Based on
/// [Ed Catmur, _Higher-Order Template Metaprogramming with
/// C++23_](https://www.youtube.com/watch?v=KENynEQoqCo).
template <typename T, auto C>
concept InputRangeOver
    = std::ranges::input_range<T> and Satisfies<std::ranges::range_value_t<T>, C>;

/// @brief Satisified if `A` and `B` can be compared using predicate `Pred`.
template <typename A, typename B, typename Pred = std::ranges::equal_to>
concept DirectlyComparable = requires(Pred pred, A a, B b) {
    { std::invoke(pred, a, b) } -> std::convertible_to<bool>;
};

/// @brief Satisified if `A` and `B` are ranges, which can be compared using `std::ranges::equal`
/// with predicate `Pred`.
template <typename A, typename B, typename Pred = std::ranges::equal_to>
concept RangeComparable = requires(Pred pred, A a, B b) {
    { std::ranges::equal(a, b, pred) } -> std::convertible_to<bool>;
};

/// Satisfied if `T` is `std::array` of any type and size.
template <typename T>
concept IsStdArray = std::same_as<T, std::array<typename T::value_type, sizeof(T)>>;

/// Satisfied if `T::value_type` is an arithmetic type.
template <typename T>
concept HasArithmeticValue = requires {
    typename T::value_type;
    requires Arithmetic<typename T::value_type>;
};

/// @brief Satisfied if type can be serialized regardless of endianness.
///
/// Satisfied for arithmetic types and arrays of them (bounded C-style and `std::array`).
template <typename T>
concept BiendianSerializable
    = Arithmetic<T> || (std::is_bounded_array_v<T> && Arithmetic<std::remove_extent_t<T>>)
    || (IsStdArray<T> && HasArithmeticValue<T>);

/// @brief User-defined literal operator producing `uint8_t`.
constexpr uint8_t operator"" _u8(unsigned long long a) noexcept { return static_cast<uint8_t>(a); }

/// @brief Check if `x` is a nonnegative finite number.
/// @param x floating-point number to check
constexpr bool is_bad_or_neg(double x) { return std::signbit(x) || !std::isfinite(x); }

/// @brief Throw if `x` is negative or infinite.
/// @param x floating-point number to check
/// @throws std::runtime_error if `x` is negative or infinite.
constexpr void throw_bad_neg(double x)
{
    if (is_bad_or_neg(x))
        throw std::runtime_error{ "parameter must be nonnegative and finite" };
}

/// `static_assert` to make sure that endianness is little or big, not mixed.
consteval void mixed_endianness_check()
{
    static_assert(std::endian::native == std::endian::little
                      || std::endian::native == std::endian::big,
                  "Mixed endianness architectures are not supported");
}

/// @brief Copy object representation of T to a byte array.
/// @tparam T a [trivially copyable](https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable)
/// type of `data`
/// @tparam B desired byte representation - `uint8_t` or `std::byte`
/// @param data data to copy as byte array
/// @return Byte array containing object representation of `data`.
template <TriviallyCopyable T, ByteRepr B = std::uint8_t>
constexpr std::array<B, sizeof(T)> to_bytes(const T &data) noexcept
{
    mixed_endianness_check();
    static_assert(BiendianSerializable<T> || std::endian::native == std::endian::little,
                  "Only little-endian architecture is supported for non-arithmetic types");
    static_assert(!std::same_as<T, std::string_view>,
                  "string_view is a pointer-length pair, not data");
    static_assert(sizeof(std::array<B, sizeof(T)>) == sizeof(B[sizeof(T)]));

    if constexpr (std::is_bounded_array_v<T>) {
        auto as_arr = std::to_array(data);
        return to_bytes<decltype(as_arr), B>(as_arr);
    }
    if constexpr (IsStdArray<T>) {
        std::array<B, sizeof(T)> result;
        auto output_iter = result.begin();
        for (std::size_t i = 0; i < data.size(); ++i) {
            const auto converted_elem = to_bytes<typename T::value_type, B>(data[i]);
            output_iter = std::copy_n(converted_elem.begin(), sizeof converted_elem, output_iter);
        }
        return result;
    }

    auto result = std::bit_cast<std::array<B, sizeof(T)>>(data);
    if constexpr (std::is_arithmetic_v<T> && std::endian::native == std::endian::big)
        std::ranges::reverse(result);
    return result;
}

/// @brief Copy object representation of items in `r` to a byte vector.
/// @tparam R `input_range` type
/// @tparam B desired byte representation - `uint8_t` or `std::byte`
/// @param r range to copy as bytes
/// @return `std::vector` of bytes containing object representation of `r`'s items.
template <std::ranges::input_range R, ByteRepr B = std::uint8_t>
// requires InputRangeOver<R, []<TriviallyCopyable> {}> // C/C++ extension in VS Code complains
    requires TriviallyCopyable<std::ranges::range_value_t<R>>
constexpr std::vector<B> range_to_bytes(R &&r)
{
    return r | std::views::transform([](auto c) { return to_bytes(c); }) | std::views::join
        | std::ranges::to<std::vector<B>>();
}

/// @brief Copy bytes from `from` array into a trivially copyable type `To` object.
/// @tparam To a [trivially copyable](https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable)
/// type
/// @tparam B type of byte representation in source array
/// @param from an array of bytes to convert into `To`
/// @return An instance of `To` whose object representation is `from`.
template <TriviallyCopyable To, ByteRepr B>
constexpr To from_bytes(const std::array<B, sizeof(To)> &from) noexcept
{
    mixed_endianness_check();
    static_assert(std::is_arithmetic_v<To> || std::endian::native == std::endian::little,
                  "Only little-endian architecture is supported for non-arithmetic types");
    static_assert(sizeof(std::array<B, sizeof(To)>) == sizeof(B[sizeof(To)]));

    if constexpr (std::is_arithmetic_v<To> && std::endian::native == std::endian::big) {
        std::array<B, sizeof(To)> reversed(from);
        std::ranges::reverse(reversed);
        return std::bit_cast<To>(reversed);
    }
    return std::bit_cast<To>(from);
}

/// @brief Copy bytes from `from` array into an `N`-element array of `To`.
/// @tparam To an arithmetic type of output elements
/// @tparam N number of `To` elements in output array
/// @tparam B type of byte representation in source array
/// @param from `std::array` with source bytes; must have size of `N * sizeof(To)`
/// @return `std::array` of `N` `To` elements.
template <Arithmetic To, std::size_t N, ByteRepr B>
constexpr std::array<To, N> array_from_bytes(const std::array<B, N * sizeof(To)> &from) noexcept
{
    mixed_endianness_check();
    static_assert(sizeof(std::array<B, N * sizeof(To)>) == sizeof(B[N * sizeof(To)]));

    auto result = std::bit_cast<std::array<To, N>>(from);
    if constexpr (std::endian::native == std::endian::big) {
        for (auto &e : result) {
            // It probably can be done with less copying, but I don't care about big endian
            // architectures
            auto elem_bytes = std::bit_cast<std::array<std::byte, sizeof(To)>>(e);
            std::ranges::reverse(elem_bytes);
            e = std::bit_cast<To>(elem_bytes);
        }
    }
    return result;
}

/// @brief Create instance of `To` using bytes from `from`
/// @tparam To a [trivially copyable](https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable)
/// type
/// @tparam R `input_range` over type satisfying ByteRepr
/// @param from bytes to convert to `To`; must have at least `sizeof(To)` elements
/// @return `To` constructed from first `sizeof(To)` bytes of input range.
template <TriviallyCopyable To, std::ranges::input_range R>
    requires ByteRepr<std::ranges::range_value_t<R>>
constexpr To from_byte_range(const R &from)
{
    using B = std::ranges::range_value_t<R>;
    if (std::ranges::size(from) < sizeof(To))
        throw std::runtime_error{ "Range is too short for the type" };

    std::array<B, sizeof(To)> tmp{};
    std::ranges::copy_n(std::ranges::cbegin(from), sizeof(To), tmp.begin());
    return from_bytes<To>(tmp);
}

/// @brief Recursively concatenate input ranges (over same type) into one vector.
///
/// In C++26 there will be `std::views::concat` ([P2542R8](https://wg21.link/P2542)), which could be
/// used e.g. with `std::ranges::to` to do the same. So far it's only supported by [libstdc++
/// 15](https://gcc.gnu.org/gcc-15/changes.html#libstdcxx). Progress in other implementations:
///
/// - https://github.com/llvm/llvm-project/issues/105419
/// - https://github.com/microsoft/STL/issues/4514
///
/// @param a first input range
/// @param b second input range
/// @param ...others remaining input ranges (if any)
/// @return `std::vector` with elements from all ranges passed as arguments.
template <std::ranges::input_range A, std::ranges::input_range B,
          std::ranges::input_range... Others>
    requires std::same_as<std::ranges::range_value_t<A>, std::ranges::range_value_t<B>>
constexpr std::vector<std::ranges::range_value_t<A>> concat_iterables(const A &a, const B &b,
                                                                      const Others &...others)
{
    const auto s = std::ranges::size(a) + std::ranges::size(b);
    std::vector<std::ranges::range_value_t<A>> result{};
    result.reserve(s);
#if __cpp_lib_containers_ranges >= 202202L
    result.append_range(a);
    result.append_range(b);
#else
    std::ranges::copy(a, std::back_inserter(result));
    std::ranges::copy(b, std::back_inserter(result));
#endif
    if constexpr (sizeof...(Others) == 0UL)
        return result;
    else
        return concat_iterables(result, others...);
}

/// @brief Check if `bin_data` starts with the given prefix.
///
/// The prefix is converted to a range of `uint8_t` before comparing.
///
/// Comparison uses `std::ranges::starts_with` if available (checked using
/// `__cpp_lib_ranges_starts_ends_with` macro, which is not defined in libc++ 17 and 18 even though
/// it implements this function; a workaround is made in define_fixes.hpp).<br>
/// In other cases `std::ranges::mismatch` is used.
/// @tparam T input range over bytes
/// @param prefix expected data prefix, specified as `std::string_view`
/// @param bin_data data to check
template <std::ranges::input_range T>
    requires ByteRepr<std::ranges::range_value_t<T>>
constexpr bool prefix_match(const std::string_view prefix, const T &bin_data)
{
    if (std::ranges::size(bin_data) < prefix.size())
        return false;
    const auto prefix_bin = std::views::transform(
        prefix, [](const char c) constexpr { return static_cast<std::uint8_t>(c); });
#if __cpp_lib_ranges_starts_ends_with >= 202106L
    return std::ranges::starts_with(bin_data, prefix_bin);
#else
    return std::ranges::mismatch(bin_data, prefix_bin).in2 == std::ranges::end(prefix_bin);
#endif
}

#ifdef LAB_TESTS
#include <charconv>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <string>

using namespace std::string_view_literals;

/// @brief Read a test data CSV
/// @tparam Path something accepted by `std::ifstream` constructor
/// @param filename path to the test file
/// @param skiplines number of lines to ignore (e.g. headers, other params)
/// @return A map of time (`int`) to expected result (`double`).
template <typename Path>
std::map<int, double> read_test_data(const Path &filename, std::size_t skiplines = 0)
{
    std::map<int, double> result;
    std::string line;
    std::ifstream data_file{ filename };
    if (!data_file.is_open())
        throw std::runtime_error{ "Test data file not found" };
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

/// @brief Compare floating point numbers with tolerance
/// @tparam F type of the floating point numbers
/// @tparam tolerance tolerance threshold of type F
/// @param a first number
/// @param b second number
/// @return `true` if the absolute difference is not greater than tolerance
template <std::floating_point F, F tolerance = F{}> constexpr bool floating_eq(F a, F b)
{
    return std::fabs(a - b) <= tolerance;
}

/// @brief Log and run tests, passing criteria is not throwing, writes result to `std::cerr`
/// @tparam F an invocable type
/// @param test_name string to log as the test name
/// @param test test function/lambda
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

/// @brief Log and run tests, passing criteria is throwing an appropriate exception, writes result
/// to `std::cerr`
///
/// When the test fails an appriopriate message is logged:
/// - no exception
/// - correct type, wrong message - logs expected and actual message
/// - wrong `std::exception` derived type - logs expected and actual type and message
/// - wrong **NOT `std::exception`** type - logs expected type and message with information about
/// unknown error
///
/// @tparam E the expected exception type
/// @param test_name string to log as the test name
/// @param test test function/lambda
/// @param what expected error message (`e.what()`), may be `nullopt` if the message should not be
/// checked
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
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexceptions"
#elif __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexceptions"
#endif
    } catch (const std::exception &e) {
#ifdef __clang__
#pragma clang diagnostic pop
#elif __GNUC__
#pragma GCC diagnostic pop
#endif
        log_different();
        std::cerr << "\n\tCaught " << typeid(e).name() << " with reason: " << e.what() << '\n';
    } catch (...) {
        log_different();
        std::cerr << "\n\tCaught unexpected exception not derived from std::exception\n";
    }
}

/// @brief Log and run tests, passing criteria is returning the expected value, writes result to
/// `std::cerr`
/// @tparam T type of value to compare to, may be a range
/// @tparam Pred type of predicate used for value comparison
/// @tparam F an invocable type returning something comparable to T
/// @param test_name string to log as the test name
/// @param value expected value or a range of values
/// @param test test function/lambda
/// @param pred predicate used for comparing actual and expected results
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
        matches = std::ranges::size(value) == std::ranges::size(result)
            && std::ranges::equal(value, result, pred);
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
#endif
