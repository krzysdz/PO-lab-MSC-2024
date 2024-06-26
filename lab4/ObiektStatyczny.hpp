#pragma once
#include "../lab1/ObiektSISO.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

class ObiektStatyczny : public ObiektSISO {
public:
    constexpr static std::string_view unique_name{ "Stat" };

private:
    static constexpr std::size_t prefix_size{ unique_name.size()
                                              * sizeof(decltype(unique_name)::value_type) };
    double m_max_val;
    double m_min_val;
    double m_a;
    double m_b;

public:
    using point = std::pair<double, double>;

    constexpr ObiektStatyczny(point p1 = { -1.0, -1.0 }, point p2 = { 1.0, 1.0 })
    {
        set_points(p1, p2);
    }
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
