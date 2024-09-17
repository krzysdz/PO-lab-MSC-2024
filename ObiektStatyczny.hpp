#pragma once
#include "ObiektSISO.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

/// Bounded (y_min, y_max) linear function derived from ObiektSISO.
class ObiektStatyczny : public ObiektSISO {
public:
    /// Unique name/prefix used to distinguish types in deserialization.
    constexpr static std::string_view unique_name{ "Stat" };

private:
    /// Size of the unique prefix.
    static constexpr std::size_t prefix_size{ unique_name.size()
                                              * sizeof(decltype(unique_name)::value_type) };
    /// Maximal output value
    double m_max_val;
    /// Minimal output value
    double m_min_val;
    /// Multiplier
    double m_a;
    /// Offset
    double m_b;

public:
    /// <x, y> pair describing a 2D point
    using point = std::pair<double, double>;

    /// @brief Regular constructor accepting 2 points describing the linear function as well as the
    /// min/max values.
    ///
    /// The points must have distinct x values. Outputs are clamped between `min(p1.y, p2.y)` and
    /// `max(p1.y, p2.y)`.
    ///
    /// @param p1 one point
    /// @param p2 the other point
    constexpr ObiektStatyczny(point p1 = { -1.0, -1.0 }, point p2 = { 1.0, 1.0 })
    {
        set_points(p1, p2);
    }
    /// @brief Deserializing constructor from an input range of bytes.
    /// @param serialized input range of bytes, whichc represent serialized ObiektStatyczny
    constexpr ObiektStatyczny(const std::ranges::input_range auto &serialized)
    {
        using B = std::ranges::range_value_t<decltype(serialized)>;
        static_assert(ByteRepr<B>, "Range must be a range over bytes");
        constexpr std::size_t expected_data_size = 4 * sizeof(double);
        constexpr std::size_t len_size = sizeof(uint32_t);
        constexpr std::size_t expected_total_size = len_size + prefix_size + expected_data_size;

        if (std::ranges::size(serialized) < expected_total_size)
            throw std::runtime_error{ "Data size is smaller than expected" };
        if (!prefix_match(unique_name, serialized | std::views::drop(len_size)))
            throw std::runtime_error{
                "ObiektStatyczny serialized data does not start with the expected prefix"
            };

        std::array<B, expected_data_size> data_bytes;
        std::ranges::copy_n(std::ranges::cbegin(serialized) + prefix_size + len_size,
                            expected_data_size, data_bytes.begin());
        const auto [max, min, a, b] = array_from_bytes<double, 4>(data_bytes);
        m_max_val = max;
        m_min_val = min;
        m_a = a;
        m_b = b;
    }
    /// Change object's/function's characteristics to match new points.
    constexpr void set_points(point p1 = { -1.0, -1.0 }, point p2 = { 1.0, 1.0 })
    {
        const auto [x1, y1] = p1;
        const auto [x2, y2] = p2;
        if (x1 == x2)
            throw std::runtime_error{ "x coordinates of both points are identical" };
        m_a = (y1 - y2) / (x1 - x2);
        m_b = y1 - m_a * x1;
        if (y1 > y2) {
            m_max_val = y1;
            m_min_val = y2;
        } else {
            m_max_val = y2;
            m_min_val = y1;
        }
    }
    /// @brief Get points describing the object
    /// @return A pair of points which describe the function's behaviour. May slightly differ from
    /// the one given initally. First point has smaller `y` value.
    constexpr std::pair<point, point> get_points() const noexcept
    {
        return { point{ (m_min_val - m_b) / m_a, m_min_val },
                 point{ (m_max_val - m_b) / m_a, m_max_val } };
    }
    /// @brief Simulate clamped linear function.
    ///
    /// @f[
    ///   f(u) = \min(max\_val, \max(min\_val, a \times u + b))
    /// @f]
    ///
    /// @param u function input
    /// @return clamped and scaled output
    constexpr double symuluj(double u) override
    {
        return std::min(m_max_val, std::max(m_min_val, m_a * u + m_b));
    }
    constexpr std::vector<uint8_t> dump() const override
    {
        auto bytes = to_bytes(std::array{ m_max_val, m_min_val, m_a, m_b });
        constexpr auto len_b = to_bytes(static_cast<uint32_t>(prefix_size + bytes.size()));
        return concat_iterables(len_b, range_to_bytes(unique_name), bytes);
    }
};
DESERIALIZABLE_SISO(ObiektStatyczny);
