#pragma once
#include "../lab1/ObiektSISO.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

class ObiektStatyczny : public ObiektSISO {
private:
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
};
