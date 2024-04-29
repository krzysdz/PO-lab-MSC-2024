#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

template <typename T>
concept trivially_copyable = std::is_trivially_copyable_v<T>;

template <typename T>
concept arithmetic = std::is_arithmetic_v<T>;

template <typename B>
concept byte_repr = std::is_same_v<B, unsigned char> || std::is_same_v<B, std::byte>;

template <typename> struct is_std_array : std::false_type { };
template <typename T, std::size_t N> struct is_std_array<std::array<T, N>> : std::true_type { };
template <typename T> constexpr bool is_std_array_v = is_std_array<T>::value;

template <typename T, typename Enable = void> struct has_arithmetic_value : std::false_type { };
template <typename T>
struct has_arithmetic_value<T, std::enable_if_t<std::is_arithmetic_v<typename T::value_type>>>
    : std::true_type { };
template <typename T> constexpr bool has_arithmetic_value_v = has_arithmetic_value<T>::value;

template <typename T>
constexpr bool biendian_serializable_v = std::is_arithmetic_v<T>
    || (std::is_bounded_array_v<T> && std::is_arithmetic_v<std::remove_extent_t<T>>)
    || (is_std_array_v<T> && has_arithmetic_value_v<T>);

constexpr uint8_t operator"" _u8(unsigned long long a) noexcept { return static_cast<uint8_t>(a); }

constexpr bool is_bad_or_neg(double x) { return std::signbit(x) || !std::isfinite(x); }

constexpr void throw_bad_neg(double x)
{
    if (is_bad_or_neg(x))
        throw std::runtime_error{ "parameter must be nonnegative and finite" };
}

consteval void mixed_endianness_check()
{
    static_assert(std::endian::native == std::endian::little
                      || std::endian::native == std::endian::big,
                  "Mixed endianness architectures are not supported");
}

template <trivially_copyable T, byte_repr B = std::uint8_t>
constexpr std::array<B, sizeof(T)> to_bytes(const T &data) noexcept
{
    mixed_endianness_check();
    static_assert(biendian_serializable_v<T> || std::endian::native == std::endian::little,
                  "Only little-endian architecture is supported for non-arithmetic types");

    if constexpr (std::is_bounded_array_v<T>) {
        auto as_arr = std::to_array(data);
        return to_bytes<decltype(as_arr), B>(as_arr);
    }
    if constexpr (is_std_array_v<T>) {
        std::array<B, sizeof(T)> result;
        auto output_iter = result.begin();
        for (std::size_t i = 0; i < data.size(); ++i) {
            const auto converted_elem = to_bytes<typename T::value_type, B>(data[i]);
            output_iter = std::copy_n(converted_elem.begin(), sizeof(converted_elem), output_iter);
        }
        return result;
    }

    auto result = std::bit_cast<std::array<B, sizeof(T)>>(data);
    if constexpr (std::is_arithmetic_v<T> && std::endian::native == std::endian::big)
        std::ranges::reverse(result);
    return result;
}

template <trivially_copyable To, byte_repr B>
constexpr To from_bytes(const std::array<B, sizeof(To)> &from) noexcept
{
    mixed_endianness_check();
    static_assert(std::is_arithmetic_v<To> || std::endian::native == std::endian::little,
                  "Only little-endian architecture is supported for non-arithmetic types");

    if constexpr (std::is_arithmetic_v<To> && std::endian::native == std::endian::big) {
        std::array<B, sizeof(To)> reversed(from);
        std::ranges::reverse(reversed);
        return std::bit_cast<To>(reversed);
    }
    return std::bit_cast<To>(from);
}

template <arithmetic To, std::size_t N, byte_repr B>
constexpr std::array<To, N> array_from_bytes(const std::array<B, N * sizeof(To)> &from) noexcept
{
    mixed_endianness_check();

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
